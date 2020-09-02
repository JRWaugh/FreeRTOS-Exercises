#include "GCodeParser.h"

GCodeParser::GCodeParser(PlotterInterface* plotter) : plotter{ plotter } {}

void GCodeParser::parse(char const* g_code) {
    switch (g_code[0]) {
    case 'G':
        switch (std::atoi(g_code + 1)) {
        case 1: {
            float x{ 0 }, y{ 0 };
            uint8_t relative{ 0 };

            if (std::sscanf(g_code + 3, "X%f Y%f A%c", &x, &y, &relative) == 3) {
                if (plotter != nullptr)
                    plotter->onG1Received(x, y, relative);
            } else {
                if (plotter != nullptr)
                    plotter->onError(kMalformedCode);
            }
            break;
        }

        case 28:
            if (plotter != nullptr)
                plotter->onG28Received();
            break;

        default:
            if (plotter != nullptr)
                plotter->onError(kUnknownCode);
            break;
        }
        break;

    case 'M':
        switch (std::atoi(g_code + 1)) {
        case 1: {
            uint8_t pen_position{ 0 };

            if (std::sscanf(g_code + 3, "%c", &pen_position) == 1) {
                if (plotter != nullptr)
                    plotter->onM1Received(pen_position);
            } else {
                if (plotter != nullptr)
                    plotter->onError(kMalformedCode);
            }
            break;
        }

        case 2: {
            uint8_t up{ 0 }, down{ 0 };

            if (std::sscanf(g_code + 3, "U%c D%c", &up, &down) == 2) {
                if (plotter != nullptr)
                    plotter->onM2Received(up, down);
            } else {
                if (plotter != nullptr)
                    plotter->onError(kMalformedCode);
            }
            break;
        }

        case 4: {
            uint8_t laser_power{ 0 };

            if (std::sscanf(g_code + 3, "%c", &laser_power) == 1) {
                if (plotter != nullptr)
                    plotter->onM4Received(laser_power);
            } else {
                if (plotter != nullptr)
                    plotter->onError(kMalformedCode);
            }
            break;
        }

        case 5: {
            uint8_t a_step{ 0 }, b_step{ 0 }, speed{ 0 };
            uint32_t height{ 0 }, width{ 0 };

            if (std::sscanf(g_code + 3, "A%c B%c H%ld W%ld S%c", &a_step, &b_step, &height, &width, &speed) == 5) {
                if (plotter != nullptr)
                    plotter->onM5Received(a_step, b_step, height, width, speed);
            } else {
                if (plotter != nullptr)
                    plotter->onError(kMalformedCode);
            }
            break;
        }

        case 10:
            if (strlen(g_code) == 3) // Probably not necessary when reading mdraw codes from serial, but we need to skip M10 replies in logs
                if (plotter != nullptr)
                    plotter->onM10Received();
            break;

        case 11:
            if (plotter != nullptr)
                plotter->onM11Received();
            break;

        default:
            if (plotter != nullptr)
                plotter->onError(kUnknownCode);
            break;
        }
        break;

    default:
        if (plotter != nullptr)
            plotter->onError(kNotAGCode);
        break;
    }
}
