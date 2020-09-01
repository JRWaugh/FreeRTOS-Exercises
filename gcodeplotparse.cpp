#include <cstdint> 
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

class PlotterInterface {
public:
    constexpr static auto OK = "OK\r\n";

    virtual ~PlotterInterface() {};
    virtual void onM1Received(uint8_t pen_position) = 0;
    virtual void onM2Received(uint8_t pen_up, uint8_t pen_down) = 0;
    virtual void onM4Received(uint8_t laser_power) = 0;
    virtual void onM5Received(uint8_t a_step, uint8_t b_step, uint32_t height, uint32_t width, uint8_t speed) = 0;
    virtual void onM10Received() const = 0;
    virtual void onM11Received() const = 0;
    virtual void onG1Received(float x, float y, uint8_t relative) = 0;
    virtual void onG28Received() const = 0;
    virtual void onError(char const * reason) const = 0;
};

class DebugPlotter : public PlotterInterface {
    constexpr static bool kShowErrors{ false };
    constexpr static bool kShowDebug{ false };

public:
    DebugPlotter(void (*print_func)(char const*)) : print_func{ print_func }, buffer{ 0 } {}

    void onM1Received(uint8_t pen_position) noexcept {
        if constexpr (kShowDebug) {
            snprintf(buffer, 64, "[DEBUG] M1: Pen Position %d\r\n", pen_position);
            print_func(buffer);
        }
        print_func(OK);
    }

    void onM2Received(uint8_t pen_up, uint8_t pen_down) noexcept {
        if constexpr (kShowDebug) {
            snprintf(buffer, 64, "[DEBUG] M2: Pen Up %d, Pen Down %d\r\n", pen_up, pen_down);
            print_func(buffer);
        }
        print_func(OK);
    }

    void onM4Received(uint8_t laser_power) noexcept {
        if constexpr (kShowDebug) {
            snprintf(buffer, 64, "[DEBUG] M4: Laser Power %d\r\n", laser_power);
            print_func(buffer);
        }
        print_func(OK);
    }

    void onM5Received(uint8_t a_step, uint8_t b_step, uint32_t height, uint32_t width, uint8_t speed) noexcept {
        if constexpr (kShowDebug) {
            snprintf(buffer, 64, "[DEBUG] M5: A Step %d, B Step %d, Height %d, Width %d, Speed %d\r\n", a_step, b_step, height, width, speed);
            print_func(buffer);
        }
        print_func(OK);
    }

    void onM10Received() const noexcept {
        if constexpr (kShowDebug)
            print_func("[DEBUG] M10: Sending dummy plotter details.\r\n");
        print_func("XY 380 310 0.00 0.00 A0 B0 H0 S80 U160 D90\r\n");
        print_func(OK);
    }

    void onM11Received(void) const noexcept {
        if constexpr (kShowDebug)
            print_func("[DEBUG] M11: Sending dummy limit switch details.\r\n");
        print_func("M11 1 1 1 1\r\n");
        print_func(OK);
    }

    void onG1Received(float x, float y, uint8_t relative) noexcept {
        if constexpr (kShowDebug) {
            snprintf(buffer, 64, "[DEBUG] G1: X%.2f, Y%.2f, Relative %c\r\n", x, y, relative);
            print_func(buffer);
        }
        print_func(OK);
    }

    void onG28Received(void) const noexcept {
        if constexpr (kShowDebug)
            print_func("[DEBUG] G28: Let's imagine we're returning to origin.\r\n");
        print_func(OK);
    }

    void onError(char const * reason) const noexcept {
        if constexpr (kShowErrors) {
            print_func("Error occurred! Reason: ");
            print_func(reason);
        }
    }
    
private:
    void (*print_func)(char const* buffer);
    char buffer[64];
};

class GCodeParser {
public:
    GCodeParser(PlotterInterface * const plotter) : plotter{ plotter } {};
    
    void parse(char const* g_code) {
        switch (g_code[0]) {
        case 'G':
            switch (std::atoi(g_code + 1)) {
            case 1: {
                float x{ 0 }, y{ 0 };
                uint8_t relative{ 0 };

                if (std::sscanf(g_code + 3, "X%f Y%f A%c", &x, &y, &relative) == 3)
                    plotter->onG1Received(x, y, relative);
                else
                    plotter->onError(kMalformedCode);
                break;
            }

            case 28:
                plotter->onG28Received();
                break;

            default:
                plotter->onError(kUnknownCode);
                break;
            }
            break;

        case 'M':
            switch (std::atoi(g_code + 1)) {
            case 1: {
                uint8_t pen_position{ 0 };

                if (std::sscanf(g_code + 3, "%c", &pen_position) == 1)
                    plotter->onM1Received(pen_position);
                else
                    plotter->onError(kMalformedCode);
                break;
            }

            case 2: {
                uint8_t up{ 0 }, down{ 0 };

                if (std::sscanf(g_code + 3, "U%c D%c", &up, &down) == 2)
                    plotter->onM2Received(up, down);
                else
                    plotter->onError(kMalformedCode);
                break;
            }

            case 4: {
                uint8_t laser_power{ 0 };

                if (std::sscanf(g_code + 3, "%c", &laser_power) == 1)
                    plotter->onM4Received(laser_power);
                else
                    plotter->onError(kMalformedCode);
                break;
            }

            case 5: {
                uint8_t a_step{ 0 }, b_step{ 0 }, speed{ 0 };
                uint32_t height{ 0 }, width{ 0 };

                if (std::sscanf(g_code + 3, "A%c B%c H%d W%d S%c", &a_step, &b_step, &height, &width, &speed) == 5)
                    plotter->onM5Received(a_step, b_step, height, width, speed);
                else
                    plotter->onError(kMalformedCode);
                break;
            }

            case 10:
                if (std::strlen(g_code) == 3) // Probably not necessary when reading mdraw codes from serial, but we need to skip M10 replies in logs
                    plotter->onM10Received();
                break;

            case 11:
                plotter->onM11Received();
                break;

            default:
                plotter->onError(kUnknownCode);
                break;
            }
            break;    

        default:
            plotter->onError(kNotAGCode);
            break;
        }
    }
   
private:
    PlotterInterface* const plotter;
    constexpr static auto kMalformedCode = "Malformed code\r\n";
    constexpr static auto kUnknownCode = "Unknown code\r\n";
    constexpr static auto kNotAGCode = "Not a GCode\r\n";
};

int main() {
    DebugPlotter* plotter = new DebugPlotter([](char const* buffer) { std::cout << buffer; });
    GCodeParser parser(plotter);
    
    std::ifstream infile("log01.txt");
    std::string buffer;

    if (infile.is_open())
        while (std::getline(infile, buffer))
            parser.parse(buffer.c_str());
}