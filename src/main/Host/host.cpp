#ifndef ENABLE_UNIT_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "configuration.h"

#include "DreamcastMainNode.hpp"
#include "PlayerData.hpp"
#include "MaplePassthroughCommandParser.hpp"

#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/Usb/usb_interface.hpp"
#include "hal/Usb/TtyParser.hpp"
#include "hal/Usb/client_usb_interface.hpp"

#include <memory>
#include <algorithm>

#define MAX_DEVICES 4

const uint8_t MAPLE_HOST_ADDRESSES[MAX_DEVICES] = {0x00, 0x40, 0x80, 0xC0};

// Second Core Process
// The second core is in charge of handling communication with Dreamcast peripherals
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Wait for steady state
    sleep_ms(100);

    uint32_t numUsbControllers = get_num_usb_controllers();
    uint32_t numDevices = std::min(numUsbControllers, (uint32_t)MAX_DEVICES);

    uint32_t maplePins[MAX_DEVICES] = {
        P1_BUS_START_PIN, P2_BUS_START_PIN, P3_BUS_START_PIN, P4_BUS_START_PIN
    };
    int32_t mapleDirPins[MAX_DEVICES] = {
        P1_DIR_PIN, P2_DIR_PIN, P3_DIR_PIN, P4_DIR_PIN
    };
    CriticalSectionMutex screenMutexes[numDevices];
    std::shared_ptr<ScreenData> screenData[numDevices];
    std::shared_ptr<PlayerData> playerData[numDevices];
    DreamcastControllerObserver** observers = get_usb_controller_observers();
    std::shared_ptr<MapleBusInterface> buses[numDevices];
    std::shared_ptr<DreamcastMainNode> dreamcastMainNodes[numDevices];
    std::shared_ptr<PrioritizedTxScheduler> schedulers[numDevices];
    Clock clock;
    for (uint32_t i = 0; i < numDevices; ++i)
    {
        screenData[i] = std::make_shared<ScreenData>(screenMutexes[i]);
        playerData[i] = std::make_shared<PlayerData>(i,
                                                     *(observers[i]),
                                                     *screenData[i],
                                                     clock,
                                                     usb_msc_get_file_system());
        buses[i] = create_maple_bus(maplePins[i], mapleDirPins[i], DIR_OUT_HIGH);
        schedulers[i] = std::make_shared<PrioritizedTxScheduler>(MAPLE_HOST_ADDRESSES[i]);
        dreamcastMainNodes[i] = std::make_shared<DreamcastMainNode>(
            *buses[i],
            *playerData[i],
            schedulers[i]);
    }

    // Initialize CDC to Maple Bus interfaces
    Mutex ttyParserMutex;
    TtyParser* ttyParser = usb_cdc_create_parser(&ttyParserMutex, 'h');
    ttyParser->addCommandParser(
        std::make_shared<MaplePassthroughCommandParser>(
            &schedulers[0], MAPLE_HOST_ADDRESSES, numDevices));

    while(true)
    {
        // Process each main node
        for (uint32_t i = 0; i < numDevices; ++i)
        {
            // Worst execution duration of below is ~350 us at 133 MHz when debug print is disabled
            dreamcastMainNodes[i]->task(time_us_64());
        }
        // Process any waiting commands in the TTY parser
        ttyParser->process();
    }
}

// First Core Process
// The first core is in charge of initialization and USB communication
int main()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    set_usb_descriptor_number_of_gamepads(SELECTED_NUMBER_OF_DEVICES);

#if SHOW_DEBUG_MESSAGES
    stdio_uart_init();
#endif

    multicore_launch_core1(core1);

    Mutex fileMutex;
    Mutex cdcStdioMutex;
    usb_init(&fileMutex, &cdcStdioMutex);

    while(true)
    {
        usb_task();
    }
}

#endif
