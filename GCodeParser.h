#pragma once
#include <vector>
#include <algorithm>
#include "PlotterInterface.h"

class GCodeParser {
public:
    void parse(char const* g_code);

    void attach(PlotterInterface* plotter) noexcept;

    void detach(PlotterInterface* plotter) noexcept;

private:
    std::vector<PlotterInterface*> plotters;
    PlotterInterface* plotter;

    constexpr static auto kMalformedCode = "Malformed code\r\n";
    constexpr static auto kUnknownCode = "Unknown code\r\n";
    constexpr static auto kNotAGCode = "Not a GCode\r\n";
};

