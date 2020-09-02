#ifndef PLOTTERDEBUG_H_
#define PLOTTERDEBUG_H_

#include "PlotterInterface.h"
#include "GCodeParser.h"
#include <cstdio>

class PlotterDebug : public PlotterInterface {
    constexpr static bool kShowErrors{ false };
    constexpr static bool kShowDebug{ false };

public:
    PlotterDebug(void (*print_func)(char const*) = [](char const* buffer) {});

    void onM1Received(uint8_t pen_position) noexcept;
    void onM2Received(uint8_t pen_up, uint8_t pen_down) noexcept;
    void onM4Received(uint8_t laser_power) noexcept;
    void onM5Received(uint8_t a_step, uint8_t b_step, uint32_t height, uint32_t width, uint8_t speed) noexcept;
    void onM10Received() const noexcept;
    void onM11Received(void) const noexcept;
    void onG1Received(float x, float y, uint8_t relative) noexcept;
    void onG28Received(void) const noexcept;
    void onError(char const* reason) const noexcept;

private:
    void (*print_func)(char const* buffer);
    char buffer[64]{ 0 };
};

#endif /* PLOTTERDEBUG_H_ */
