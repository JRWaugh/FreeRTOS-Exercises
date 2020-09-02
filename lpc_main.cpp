#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include <array>

#include "GCodeParser.h"
#include "PlotterDebug.h"

int main(void) {
    SystemCoreClockUpdate();
    Board_Init();
    heap_monitor_setup();

    xTaskCreate([](auto) {
        PlotterDebug plotter([](auto buffer) { Board_UARTPutSTR(buffer); });
        GCodeParser parser(&plotter);
        std::array<char, 64> buffer;
        auto count = buffer.begin();

        while (true) {
            if (int in = Board_UARTGetChar(); in != EOF) {
                *count++ = in;

                if (in == '\n' || in =='\r' || count == buffer.end()) {
                    *--count = '\0';
                    parser.parse(buffer.data());
                    count = buffer.begin();
                }
            }
        }
    }, "vTaskUart", configMINIMAL_STACK_SIZE + 128, nullptr, tskIDLE_PRIORITY + 1UL, nullptr);

    vTaskStartScheduler();

    return 1;
}
