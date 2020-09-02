#ifndef GCODEPARSER_H_
#define GCODEPARSER_H_

#include "PlotterInterface.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

class GCodeParser {
public:
    GCodeParser(PlotterInterface* plotter = nullptr);
    void parse(char const* g_code);

private:
    PlotterInterface* plotter;

    constexpr static auto kMalformedCode = "Malformed code\r\n";
    constexpr static auto kUnknownCode = "Unknown code\r\n";
    constexpr static auto kNotAGCode = "Not a GCode\r\n";
};

#endif /* GCODEPARSER_H_ */
