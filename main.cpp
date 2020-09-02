#include <iostream>
#include <fstream>
#include <string>

#include "GCodeParser.h"
#include "PlotterDebug.h"

int main() {
    PlotterDebug plotter([](auto buffer) { std::cout << buffer; });
    GCodeParser parser(&plotter);
    std::ifstream infile("log01.txt");
    std::string buffer;

    if (infile.is_open())
        while (std::getline(infile, buffer))
            parser.parse(buffer.c_str());
}
