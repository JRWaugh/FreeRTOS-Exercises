/*
 * plotter_main.cpp
 *
 *  Created on: 17 Sep 2020
 *      Author: Joshua
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "DigitalIOPin.h"
#include "plotter_main.h"
#include "QueueWrapper.h"
#include <cmath>
#include "ITM_write.h"
#include <atomic>
#include "event_groups.h"

#define CALIBRATE_MAX_PPS 1

namespace Plotter {
/* Constants */
static constexpr LPCPinMap stepPinMap{ 0, 24 };
static constexpr LPCPinMap 	dirPinMap{ 1,  0 };
static constexpr LPCPinMap LSw1PinMap{ 0,  9 };
static constexpr LPCPinMap LSw2PinMap{ 0, 29 };
static constexpr size_t kInitialPPS{ 500 };
static constexpr size_t kPPSDelta{ 400 };
static constexpr size_t kHalfStepsPerRev{ 400 };

// Stuff for later?
static volatile StepSize eStepSize{ Full };
static std::atomic<int> y_pos{ 0 }, y_limit{ 0 };

// Stuff I want for now
static bool isInit{ false };
static size_t xMaximumPPS{ UINT32_MAX }, xJourneyLength{ 0 }, xTargetPPS{ kInitialPPS };
static volatile float xCurrentPPS{ kInitialPPS }, fLinearPPSAccel{ 0 };
static DigitalIOPin* direction_pin, * limitSW1, *limitSW2;
static std::atomic<size_t> xCurrentPosition{ 0 }, xRemainingJourneyLength{ 0 }, xPlotterWidth{ 0 };
static EventGroupHandle_t xPlotterFlagGroup;
static SemaphoreHandle_t xMoveComplete;
static QueueWrapper<Message, 7>* xQueue;
static pvStartTimer pvStartTimerCallback;
static pvStopTimer pvStopTimerCallback;
static TickType_t xOriginTimeStamp{ 0 }, xLimitTimeStamp{ 0 };

/*****************************************************************************
 * Private functions
 ****************************************************************************/
static void prvSetDirection(Direction eDirection) noexcept {
	direction_pin->write(eDirection);
}

[[nodiscard]] static Direction prvGetDirection() noexcept {
	return static_cast<Direction>(direction_pin->read());
}

static Direction prvToggleDirection() noexcept {
	return static_cast<Direction>(direction_pin->toggle());
}

static void prvPrintStats() {
	static char buffer[200]{ 0 };
	sprintf(buffer,
			"Maximum PPS Value: %d\r\n"
			"Maximum RPM Value: %d\r\n"
			"Fastest time: %d\r\n", xMaximumPPS, xMaximumPPS * 60 / kHalfStepsPerRev, abs(xOriginTimeStamp - xLimitTimeStamp));
	ITM_write(buffer);
}

static void prvMove() noexcept {
	xCurrentPPS = kInitialPPS;

	pvStartTimerCallback(kInitialPPS);
	xEventGroupSetBits(xPlotterFlagGroup, Plotting);

	// Wait for semaphore given by prvTerminateMove
	xSemaphoreTake(xMoveComplete, portMAX_DELAY);
	xEventGroupClearBits(xPlotterFlagGroup, Plotting);

	vTaskDelay(1); // Let the switches stabilise. The motor is stopped at this point, so it's safe.

	if (xEventGroupGetBits(xPlotterFlagGroup) & (MaxWidthFound | Calibrating))
		if (xRemainingJourneyLength == 0 && !limitSW1->read() && !limitSW2->read())
			xEventGroupSetBits(xPlotterFlagGroup, MaxPPSFound);
}

// Will be called when remaining steps reach 0 or when a limit switch is hit.
static void prvTerminateMove() {
	if (xEventGroupGetBits(xPlotterFlagGroup) & Plotting) {
		pvStopTimerCallback();

		portBASE_TYPE xHigherPriorityWoken = pdFALSE;
		xSemaphoreGiveFromISR(xMoveComplete, &xHigherPriorityWoken);
		portEND_SWITCHING_ISR(xHigherPriorityWoken);
	}
}

static void prvMoveToRelativePosition(int xOffsetFromPosition) {
	xRemainingJourneyLength = xJourneyLength = xOffsetFromPosition;

	fLinearPPSAccel = (xTargetPPS - kInitialPPS) / (xJourneyLength * 0.1f);
	if (fLinearPPSAccel > xMaximumPPS)
		fLinearPPSAccel = xMaximumPPS;
	else if (fLinearPPSAccel < 0)
		fLinearPPSAccel = 0;

	// Can check here later if xLinearPPSAccel is greater than some max value.
	prvMove();
}

static void prvMoveToAbsolutePosition(int xAbsolutePosition) {
	xRemainingJourneyLength = xJourneyLength = abs(xCurrentPosition - xAbsolutePosition);

	fLinearPPSAccel = (xTargetPPS - kInitialPPS) / (xJourneyLength * 0.1f);
	if (fLinearPPSAccel > xMaximumPPS)
		fLinearPPSAccel = xMaximumPPS;
	else if (fLinearPPSAccel < 0)
		fLinearPPSAccel = 0;

	prvMove();
}

static void prvCalibratePlotter() {
	xEventGroupSetBits(xPlotterFlagGroup, Calibrating);
	xEventGroupClearBits(xPlotterFlagGroup, PositionFound | MaxWidthFound | MaxPPSFound);

	// Move to left so we can reset step counting, then move to right to find the total width
	prvSetDirection(Clockwise);
	prvMoveToRelativePosition(INT16_MAX);
	prvSetDirection(CounterClockwise);
	prvMoveToRelativePosition(INT16_MAX);

#if CALIBRATE_MAX_PPS == 1
	while (!(xEventGroupGetBits(xPlotterFlagGroup) & MaxPPSFound)) {
		prvToggleDirection();
		xTargetPPS += kPPSDelta;
		prvMoveToRelativePosition(xPlotterWidth);
	}

	// Reset default PPS. Rework this to be less bad at some point!
	xMaximumPPS = xTargetPPS - kPPSDelta;
	xTargetPPS = kInitialPPS;

	// We've lost our position so we need to find it again.
	xEventGroupClearBits(xPlotterFlagGroup, PositionFound);
	prvSetDirection(CounterClockwise);
	prvMoveToRelativePosition(INT16_MAX);
#endif

	// Return to centre
	prvToggleDirection();
	prvMoveToAbsolutePosition(xPlotterWidth / 2);

	xEventGroupClearBits(xPlotterFlagGroup, Calibrating);
	prvPrintStats();
}

static void prvOriginSwitchHandler(void* pvParameters) {
	static SemaphoreHandle_t xOriginAlertSemaphore = xSemaphoreCreateBinary();

	limitSW1 = new DigitalIOPin{ LSw1PinMap, true, true, true, PIN_INT0_IRQn, [](bool pressed) {
		if (pressed) {
			// We can't move any further, so terminate the move
			prvTerminateMove();

			// Notify the task so that it can update the state machine and light an LED
			portBASE_TYPE xHigherPriorityWoken = pdFALSE;
			xSemaphoreGiveFromISR(xOriginAlertSemaphore, &xHigherPriorityWoken);
			portEND_SWITCHING_ISR(xHigherPriorityWoken);
		}
	}};

	while (true) {
		xSemaphoreTake(xOriginAlertSemaphore, portMAX_DELAY);
		vStop();

		if (!(xEventGroupGetBits(xPlotterFlagGroup) & PositionFound) && xEventGroupGetBits(xPlotterFlagGroup) & Calibrating) {
			xEventGroupSetBits(xPlotterFlagGroup, PositionFound);
			xCurrentPosition = 0;
		}

		xOriginTimeStamp = xTaskGetTickCount();

		Board_LED_Set(0, true);
		vTaskDelay(configTICK_RATE_HZ / 2);
		Board_LED_Set(0, false);
	}
}

static void prvLimitSwitchHandler(void* pvParameters) {
	static SemaphoreHandle_t xLimitAlertSemaphore = xSemaphoreCreateBinary();

	limitSW2 = new DigitalIOPin{ LSw2PinMap, true, true, true, PIN_INT1_IRQn, [](bool pressed) {
		if (pressed) {
			// We can't move any further, so terminate the move
			prvTerminateMove();

			// Notify the task so that it can update the state machine and light an LED
			portBASE_TYPE xHigherPriorityWoken = pdFALSE;
			xSemaphoreGiveFromISR(xLimitAlertSemaphore, &xHigherPriorityWoken);
			portEND_SWITCHING_ISR(xHigherPriorityWoken);
		}
	}};

	while (true) {
		xSemaphoreTake(xLimitAlertSemaphore, portMAX_DELAY);
		vStop();

		auto bits = xEventGroupGetBits(xPlotterFlagGroup);
		if (!(bits & MaxWidthFound) && (bits & (PositionFound | Calibrating))) {
			xEventGroupSetBits(xPlotterFlagGroup, MaxWidthFound);
			xPlotterWidth = xCurrentPosition.load();
		} else if (bits & MaxWidthFound && !(bits & PositionFound)) {
			xEventGroupSetBits(xPlotterFlagGroup, PositionFound);
			xCurrentPosition = xPlotterWidth.load();
		}

		xLimitTimeStamp = xTaskGetTickCount();

		Board_LED_Set(1, true);
		vTaskDelay(configTICK_RATE_HZ / 2);
		Board_LED_Set(1, false);
	}
}

static void prvPlotterTask(void* pvParameters) {
	vTaskDelay(100);

	while (limitSW1->read() || limitSW2->read()); // Can change this to an event at some point.

	prvCalibratePlotter();

	while (true) {
		Message message = xQueue->pop_back();
		xEventGroupWaitBits(xPlotterFlagGroup, Flag::Go, pdFALSE, pdFALSE, portMAX_DELAY);
		switch (message.command) {
		case Message::MoveLeft:
			prvSetDirection(Clockwise);
			prvMoveToRelativePosition(message.value);
			break;

		case Message::MoveRight:
			prvSetDirection(CounterClockwise);
			prvMoveToRelativePosition(message.value);
			break;

		case Message::SetPPS:
			xTargetPPS = message.value;
			break;
		}
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

void vResume() {
	xEventGroupSetBits(xPlotterFlagGroup, Flag::Go);
}
void vStop() {
	xEventGroupClearBits(xPlotterFlagGroup, Flag::Go);
}

int xGetPlotterWidth() {
	xEventGroupWaitBits(xPlotterFlagGroup, Flag::MaxWidthFound, pdFALSE, pdFALSE, portMAX_DELAY);
	return xPlotterWidth;
}

void vOnTick() noexcept {
	static DigitalIOPin xStepPin{ stepPinMap, false, false, false };

	xStepPin.write(false);

	switch(prvGetDirection()) {
	case Clockwise:
		--xCurrentPosition;
		break;

	case CounterClockwise:
		++xCurrentPosition;
		break;
	}

	if (--xRemainingJourneyLength == 0) {
		prvTerminateMove();
	} else if (xRemainingJourneyLength >= xJourneyLength * 0.90f)
		pvStartTimerCallback(xCurrentPPS += fLinearPPSAccel);
	else if (xRemainingJourneyLength <= xJourneyLength * 0.10f)
		pvStartTimerCallback(xCurrentPPS -= fLinearPPSAccel);

	xStepPin.write(true); // Takes > 300 CPU cycles to handle this function, so that's a long enough delay for the stepper with our rinky-dink MCU!
}

BaseType_t xEnqueueMessage(Message::Command command, int value) {
	return xQueue->push_back({command, value});
}

BaseType_t xEnqueueMessage(Message const & message) {
	return xQueue->push_back(message);
}

void vInit(pvStartTimer start_timer, pvStopTimer stop_timer) noexcept {
	if (!isInit) {
		ITM_init();
		pvStartTimerCallback = start_timer;
		pvStopTimerCallback = stop_timer;

		xMoveComplete = xSemaphoreCreateBinary();
		xQueue = new QueueWrapper<Message, 7>;
		xPlotterFlagGroup = xEventGroupCreate();

		direction_pin = new DigitalIOPin{ dirPinMap, false, false, false };

		xTaskCreate(prvOriginSwitchHandler, "OriginSWHandler", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);
		xTaskCreate(prvLimitSwitchHandler, "LimitSWHandler", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);
		xTaskCreate(prvPlotterTask, "Plotter Task", configMINIMAL_STACK_SIZE + 128, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);
	}
}
}


