#include "board.h"
#include "FreeRTOS.h"
#include "heap_lock_monitor.h"
#include "semphr.h"
#include "DigitalIOPin.h"
#include "Stepper.h"

#define EX1 1
#define EX2 0
#define EX3 0

SemaphoreHandle_t stepper_notify;
SemaphoreHandle_t io_event;
DigitalIOPin* limitSW1;
DigitalIOPin* limitSW2;

static void prvSetupHardware() {
	SystemCoreClockUpdate();
	Board_Init();
	heap_monitor_setup();
}

extern "C" { void vConfigureTimerForRunTimeStats() {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}}

int main(void) {
	prvSetupHardware();

	stepper_notify = xSemaphoreCreateBinary();
	io_event = xSemaphoreCreateCounting( 7, 0 );
	limitSW1 = new DigitalIOPin{ { 0, 9 }, true, true, true, PIN_INT0_IRQn };
	limitSW2 = new DigitalIOPin{ { 0, 29}, true, true, true, PIN_INT1_IRQn };

#if EX1 == 1

	xTaskCreate([](void* pvParameters) {
		limitSW1->setOnIRQCallback([](bool pressed, portBASE_TYPE* xHigherPriorityWoken) {
			Board_LED_Set(0, pressed);
			xSemaphoreGiveFromISR(stepper_notify, xHigherPriorityWoken);
		});

		limitSW2->setOnIRQCallback([](bool pressed, portBASE_TYPE* xHigherPriorityWoken) {
			Board_LED_Set(1, pressed);
			xSemaphoreGiveFromISR(stepper_notify, xHigherPriorityWoken);
		});

		while (true)
			vTaskDelay(portMAX_DELAY); // Job's done. Only creating this task because you told me to!

	}, "Task 1", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);

	xTaskCreate([](void* pvParamters) {
		auto& stepper = Stepper::getXStepper();
		constexpr auto callback = [](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) { xSemaphoreGiveFromISR(stepper_notify, xHigherPriorityWoken); };
		DigitalIOPin button1{ { 0, 8 }, true, true, true, PIN_INT2_IRQn, callback };
		DigitalIOPin button3{ { 1, 8 }, true, true, true, PIN_INT3_IRQn, callback };

		while (true) {
			xSemaphoreTake(stepper_notify, portMAX_DELAY);

			if (limitSW1->read() || limitSW2->read()) {
				stepper.halt();
			} else if (button1.read() && !button3.read()) {
				stepper.setDirection(Stepper::Clockwise);
				stepper.resume();
			} else if (!button1.read() && button3.read()) {
				stepper.setDirection(Stepper::CounterClockwise);
				stepper.resume();
			} else {
				stepper.halt();
			}
		}
	}, "Task 2", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);


#elif EX2 == 1

	xTaskCreate([](void* pvParameters) {
		limitSW1->setOnIRQCallback([](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			Board_LED_Set(0, pressed);
			xSemaphoreGiveFromISR(io_event, xHigherPriorityWoken);
		});
		limitSW2->setOnIRQCallback([](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			Board_LED_Set(1, pressed);
			xSemaphoreGiveFromISR(io_event, xHigherPriorityWoken);
		});

		while (limitSW1->read() || limitSW2->read()) {
			xSemaphoreTake(io_event, configTICK_RATE_HZ);
			Board_LED_Toggle(2);
		}

		Board_LED_Set(2, false);

		limitSW1->setOnIRQCallback([](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			Board_LED_Set(0, pressed);
			xSemaphoreGiveFromISR(stepper_notify, xHigherPriorityWoken);
		});
		limitSW2->setOnIRQCallback([](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			Board_LED_Set(1, pressed);
			xSemaphoreGiveFromISR(stepper_notify, xHigherPriorityWoken);
		});

		xSemaphoreGive(stepper_notify);

		while (true)
			vTaskDelay(portMAX_DELAY); // job's done, put task to sleep
	}, "Task 1", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);

	xTaskCreate([](void* pvParameters) {
		auto& stepper = Stepper::getXStepper();

		while (true) {
			xSemaphoreTake(stepper_notify, portMAX_DELAY);

			if (limitSW1->read() && limitSW2->read()) {
				stepper.halt();
				vTaskDelay(configTICK_RATE_HZ * 5); // No need to recheck pins because io_event will signal any changes we missed
			} else if (limitSW1->read() || limitSW2->read()) {
				stepper.toggleDirection();
				stepper.resume();
			} else {
				stepper.resume();
			}
		}
	}, "Task 2", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);

#elif EX3 == 1

	xTaskCreate([](void* pvParameters) {
		limitSW1->setOnIRQCallback([](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			xSemaphoreGiveFromISR(io_event, xHigherPriorityWoken);
			Board_LED_Set(0, pressed);
		});

		limitSW2->setOnIRQCallback([](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			xSemaphoreGiveFromISR(io_event, xHigherPriorityWoken);
			Board_LED_Set(1, pressed);
		});

		while (limitSW1->read() || limitSW2->read()) {
			xSemaphoreTake(io_event, configTICK_RATE_HZ);
			Board_LED_Toggle(2);
		}

		Board_LED_Set(2, false);

		constexpr auto callback = [](bool pressed, portBASE_TYPE* const xHigherPriorityWoken) {
			xSemaphoreGiveFromISR(io_event, xHigherPriorityWoken);
			xSemaphoreGiveFromISR(stepper_notify, xHigherPriorityWoken);
		};

		limitSW1->setOnIRQCallback(callback);
		limitSW2->setOnIRQCallback(callback);

		xSemaphoreGive(stepper_notify);

		while (true) {
			xSemaphoreTake(io_event, portMAX_DELAY);

			if (limitSW1->read()) {
				Board_LED_Set(0, true);
				vTaskDelay(configTICK_RATE_HZ);
				Board_LED_Set(0, false);
			} else if (limitSW2->read()) {
				Board_LED_Set(1, true);
				vTaskDelay(configTICK_RATE_HZ);
				Board_LED_Set(1, false);
			}
		}
	}, "Task 1", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);

	xTaskCreate([](void* pvParameters) {
		auto& stepper = Stepper::getXStepper();

		while (true) {
			xSemaphoreTake(stepper_notify, portMAX_DELAY);
			if (limitSW1->read() && limitSW2->read()) {
				stepper.halt();
			} else if (limitSW1->read() && stepper.getDirection() == Stepper::Clockwise) {
				stepper.setOrigin();
				stepper.toggleDirection();
			} else if (limitSW2->read() && stepper.getDirection() == Stepper::CounterClockwise) {
				stepper.setLimit();
				stepper.toggleDirection();
				Board_LED_Set(2, true);
				vTaskDelay(configTICK_RATE_HZ * 2);
				Board_LED_Set(2, false);
			} else {
				stepper.resume();
			}
		}
	}, "Task 2", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);

#endif

	vTaskStartScheduler();

	return 1;
}

