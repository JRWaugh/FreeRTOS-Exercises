/*
 * plotter_main.h
 *
 *  Created on: 17 Sep 2020
 *      Author: Joshua
 */

#ifndef PLOTTER_MAIN_H_
#define PLOTTER_MAIN_H_

namespace Plotter {

struct Message {
	enum Command { MoveLeft, MoveRight, SetPPS };
	Command command;
	int value;
};

enum Direction{
	CounterClockwise = 0, Clockwise
};

enum Flag{
	Go = 1 << 0, Calibrating = 1 << 1, Plotting = 1 << 2, PositionFound = 1 << 3, MaxWidthFound = 1 << 4, MaxPPSFound= 1 << 5
};

// Number of toggles to complete a step.
enum StepSize {
	Full = 1, Half = 2, Quarter = 4, Eighth = 8, Sixteenth = 16
};

using pvStartTimer = void (*)(size_t pulses_per_second);
using pvStopTimer = void (*)();
void vResume();
void vStop();
void vOnTick();
bool bMaxPPSFound();
int xGetPlotterWidth();
BaseType_t xEnqueueMessage(Message const & message);
BaseType_t xEnqueueMessage(Message::Command command, int value);
void vInit(pvStartTimer start_timer, pvStopTimer stop_timer);

}

#endif /* PLOTTER_MAIN_H_ */
