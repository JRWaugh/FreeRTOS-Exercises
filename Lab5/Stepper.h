/*
 * Stepper.h
 *
 *  Created on: 10 Sep 2020
 *      Author: Joshua
 */

#ifndef STEPPER_H_
#define STEPPER_H_

#include "board.h"
#include "DigitalIOPin.h"
#include <atomic>
#include <utility>

class Stepper {
public:
	enum Direction{ CounterClockwise, Clockwise };
	enum State{ Unknown = 0, OriginFound = 1, LimitFound = 2 };
	enum Axis{ X_Axis, Y_Axis };

	Stepper(Stepper const &)		= delete;
	void operator=(Stepper const &)	= delete;

	static Stepper& getXStepper();
	static Stepper& getYStepper();

	void resume() noexcept;
	void halt() noexcept;

	void setOrigin() {
		state |= OriginFound;
		step_count = 0;
	}

	void setLimit() {
		state |= LimitFound;
		step_limit = step_count.load();
	}

	[[nodiscard]] State getState() const noexcept;

	[[nodiscard]] Direction getDirection() const noexcept;
	void setDirection(Direction const direction) noexcept;
	void toggleDirection() noexcept;

	void setStepsPerSecond(size_t const steps_per_second) noexcept;

	void isr();

private:
	Stepper(LPCPinMap step_pin, LPCPinMap direction_pin, size_t steps_per_second, Axis axis);

	DigitalIOPin direction_pin;
	std::atomic<size_t> step_count{ 0 }, step_limit{ 0 };
	uint8_t state{ Unknown };

	static bool isInit;
	static constexpr size_t kPrescaler{ 72 };
	static constexpr size_t kTickrateHz{ 72'000'000 / kPrescaler };
	static constexpr size_t kLimitDelta{ 10 };
};

#endif /* STEPPER_H_ */
