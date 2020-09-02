#include "PlotterDebug.h"

PlotterDebug::PlotterDebug(GCodeParser* const parser, void (*print_func)(char const*)) : parser{ parser }, print_func{ print_func } {
    parser->attach(this);
}

PlotterDebug::~PlotterDebug() {
    parser->detach(this);
}

void PlotterDebug::onM1Received(uint8_t pen_position) noexcept {
    if constexpr (kShowDebug) {
        snprintf(buffer, 64, "[DEBUG] M1: Pen Position %d\r\n", pen_position);
        print_func(buffer);
    }
    print_func(OK);
}

void PlotterDebug::onM2Received(uint8_t pen_up, uint8_t pen_down) noexcept {
    if constexpr (kShowDebug) {
        snprintf(buffer, 64, "[DEBUG] M2: Pen Up %d, Pen Down %d\r\n", pen_up, pen_down);
        print_func(buffer);
    }
    print_func(OK);
}

void PlotterDebug::onM4Received(uint8_t laser_power) noexcept {
    if constexpr (kShowDebug) {
        snprintf(buffer, 64, "[DEBUG] M4: Laser Power %d\r\n", laser_power);
        print_func(buffer);
    }
    print_func(OK);
}

void PlotterDebug::onM5Received(uint8_t a_step, uint8_t b_step, uint32_t height, uint32_t width, uint8_t speed) noexcept {
    if constexpr (kShowDebug) {
        snprintf(buffer, 64, "[DEBUG] M5: A Step %d, B Step %d, Height %d, Width %d, Speed %d\r\n", a_step, b_step, height, width, speed);
        print_func(buffer);
    }
    print_func(OK);
}

void PlotterDebug::onM10Received() const noexcept {
    if constexpr (kShowDebug)
        print_func("[DEBUG] M10: Sending dummy plotter details.\r\n");
    print_func("XY 380 310 0.00 0.00 A0 B0 H0 S80 U160 D90\r\n");
    print_func(OK);
}

void PlotterDebug::onM11Received(void) const noexcept {
    if constexpr (kShowDebug)
        print_func("[DEBUG] M11: Sending dummy limit switch details.\r\n");
    print_func("M11 1 1 1 1\r\n");
    print_func(OK);
}

void PlotterDebug::onG1Received(float x, float y, uint8_t relative) noexcept {
    if constexpr (kShowDebug) {
        snprintf(buffer, 64, "[DEBUG] G1: X%.2f, Y%.2f, Relative %c\r\n", x, y, relative);
        print_func(buffer);
    }
    print_func(OK);
}

void PlotterDebug::onG28Received(void) const noexcept {
    if constexpr (kShowDebug)
        print_func("[DEBUG] G28: Let's imagine we're returning to origin.\r\n");
    print_func(OK);
}

void PlotterDebug::onError(char const* reason) const noexcept {
    if constexpr (kShowErrors) {
        print_func("Error occurred! Reason: ");
        print_func(reason);
    }
}