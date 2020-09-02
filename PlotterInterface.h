#pragma once
#include <cstdint>

struct PlotterInterface {
    constexpr static auto OK = "OK\r\n";

    virtual ~PlotterInterface() = default;

    virtual void onM1Received(uint8_t pen_position) = 0;
    virtual void onM2Received(uint8_t pen_up, uint8_t pen_down) = 0;
    virtual void onM4Received(uint8_t laser_power) = 0;
    virtual void onM5Received(uint8_t a_step, uint8_t b_step, uint32_t height, uint32_t width, uint8_t speed) = 0;
    virtual void onM10Received() const = 0;
    virtual void onM11Received() const = 0;
    virtual void onG1Received(float x, float y, uint8_t relative) = 0;
    virtual void onG28Received() const = 0;
    virtual void onError(char const* reason) const = 0;
};