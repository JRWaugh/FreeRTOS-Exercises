/*
 * Stepper.cpp
 *
 *  Created on: 10 Sep 2020
 *      Author: Joshua
 */

#include "Stepper.h"

bool Stepper::isInit{ false };

extern "C" {
void SCT2_IRQHandler(void) {
	if (LPC_SCT2->EVFLAG & SCT_EVT_0) {
		Stepper::getXStepper().isr();
		Chip_SCT_ClearEventFlag(LPC_SCT2, SCT_EVT_0);
	} else if (LPC_SCT2->EVFLAG & SCT_EVT_1) {
		Stepper::getYStepper().isr();
		Chip_SCT_ClearEventFlag(LPC_SCT2, SCT_EVT_1);
	}
}
}

Stepper& Stepper::getXStepper() {
	static Stepper X{ { 0, 24 }, { 1, 0 }, 100, X_Axis };

	return X;
}

Stepper& Stepper::getYStepper() {
	static Stepper Y{ { 0, 22 }, { 1, 0 }, 100, Y_Axis };

	return Y;
}

Stepper::Stepper(LPCPinMap step_pin, LPCPinMap direction_pin, size_t steps_per_second, Axis axis)
: direction_pin{ direction_pin, false, false, false } {
	if (!isInit) {
		Chip_SCTPWM_Init(LPC_SCT2);
		LPC_SCT2->CONFIG = 1 << 0 | 1 << 17; 	// Unified timer | auto limit
		LPC_SCT2->CTRL_U |= (kPrescaler - 1) << 5; 	// Set prescaler. Sysclock / 72 = 1MHz
		NVIC_EnableIRQ(SCT2_IRQn);
		isInit = true;
	}

	// Leaving the normal PWM stuff here for now, since a Stepper only needs a square wave.
	LPC_SCT2->MATCHREL[axis].U = kTickrateHz / (steps_per_second * 2) - 1; 			// Number of cycles required to trigger Match N
	//LPC_SCT2->MATCHREL[axis + 1].U = (kTickrateHz / 2) / steps_per_second - 1; 	// Number of cycles required to trigger Match N + 1
	LPC_SCT2->EVENT[axis].STATE = 0xFFFFFFFF; 									// States in which Event N will occur
	//LPC_SCT2->EVENT[axis + 1].STATE = 0xFFFFFFFF; 								// States in which Event N + 1 will occur
	LPC_SCT2->EVENT[axis].CTRL = axis | 1 << 12; 								// Event N will occur when Match N occurs
	//LPC_SCT2->EVENT[axis + 1].CTRL = (axis + 1) | 1 << 12; 						// Event N + 1 will occur when Match N + 1 occurs
	LPC_SCT2->EVEN = 1 << axis;													// Event N will trigger SCT_EVT_N interrupt
	LPC_SCT2->OUT[axis].SET = 1 << axis; 										// Event N will set SCT2_OUTN pins to high
	LPC_SCT2->OUT[axis].CLR = 1 << axis;
	//LPC_SCT2->OUT[axis].CLR = 1 << (axis + 1); 									// Event N + 1 will reset SCT2_OUTN pin to low
	LPC_SCT2->RES = 3 << axis;

	if (axis == X_Axis) {
		Chip_SWM_MovablePortPinAssign(SWM_SCT2_OUT0_O, step_pin.port, step_pin.pin);
	} else if (axis == Y_Axis) {
		Chip_SWM_MovablePortPinAssign(SWM_SCT2_OUT1_O, step_pin.port, step_pin.pin);
	}

	setDirection(Clockwise);
}

void Stepper::resume() noexcept {
	LPC_SCT2->CTRL_L &= ~(1 << 2);
}

void Stepper::halt() noexcept {
	LPC_SCT2->CTRL_L |= 1 << 2;
}

[[nodiscard]] Stepper::State Stepper::getState() const noexcept {
	return static_cast<State>(state);
}

[[nodiscard]] Stepper::Direction Stepper::getDirection() const noexcept {
	return static_cast<Direction>(direction_pin.read());
}

void Stepper::setDirection(Direction const direction) noexcept {
	direction_pin.write(direction);
}

void Stepper::toggleDirection() noexcept {
	direction_pin.toggle();
}


void Stepper::setStepsPerSecond(size_t const steps_per_second) noexcept {
	LPC_SCT2->MATCHREL[0].U = kTickrateHz / (steps_per_second * 2) - 1;
}

void Stepper::isr() {
	switch (getDirection()) {
	case CounterClockwise:
		if (++step_count >= step_limit - kLimitDelta && state & LimitFound)
			toggleDirection();
		break;

	case Clockwise:
		if (--step_count <= kLimitDelta && state & OriginFound)
			toggleDirection();
		break;
	}
}
