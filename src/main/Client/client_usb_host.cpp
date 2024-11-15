#ifndef ENABLE_UNIT_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/platform.h"

#include "configuration.h"
#include "pico_configurations.h"

#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"
#include "NonVolatilePicoSystemMemory.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"

#include "DreamcastMainPeripheral.hpp"
#include "DreamcastController.hpp"
#include "DreamcastStorage.hpp"
#include "DreamcastVibration.hpp"
#include "DreamcastScreen.hpp"
#include "DreamcastTimer.hpp"

#include "led.hpp"

#include <memory>
#include <algorithm>

void screenCb(const uint32_t* screen, uint32_t len)
{
    // TODO: Fill in
}

void setTimeCb(const client::DreamcastTimer::SetTime& setTime)
{
    // TODO: Fill in
}

void storageCb(uint32_t state)
{
    // Push blocking waits until data can be written to FIFO. Since we don't want to stall the core
    // which would result in dropped packets, we check to see if we can write to the FIFO (non-blocking)
    // and only write to it if it's available. Since this is being used for a very limited purpose, we
    // shouldn't be concerned that it will fill, this is just my paranoia coming out.
    if(multicore_fifo_wready())
    {
        multicore_fifo_push_blocking(state);
    }
}

void setPwmFn(uint8_t width, uint8_t down)
{
    if (width == 0 || down == 0)
    {
        buzzer.stop(2);
    }
    else
    {
        buzzer.stop(2);
        buzzer.buzzRaw({.priority=2, .wrapCount=width, .highCount=(width-down)});
    }
}

void gpio_init()
{
    // Configure ADC for triggers and stick
    adc_init();

    adc_set_clkdiv(0);
    adc_gpio_init(CTRL_PIN_SX); // Stick X
    adc_gpio_init(CTRL_PIN_SY); // Stick Y
    adc_gpio_init(CTRL_PIN_LT);  // Left Trigger
    adc_gpio_init(CTRL_PIN_RT);  // Right Trigger

    //TODO implement screen type in the DreamcastScreen class
    // OLED Select GPIO (high/open = SSD1331, Low = SSD1306)
    gpio_init(OLED_SEL_PIN);
    gpio_set_dir(OLED_SEL_PIN, false);
    gpio_pull_up(OLED_SEL_PIN);

    //TODO implement page button?

    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(ButtonInfos[i].pin);
        gpio_set_dir(ButtonInfos[i].pin, false);
        gpio_pull_up(ButtonInfos[i].pin);
    }
}

std::shared_ptr<NonVolatilePicoSystemMemory> mem =
    std::make_shared<NonVolatilePicoSystemMemory>(
        PICO_FLASH_SIZE_BYTES - client::DreamcastStorage::MEMORY_SIZE_BYTES,
        client::DreamcastStorage::MEMORY_SIZE_BYTES);

// Second Core Process
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    //usb_init();

    while (true)
    {
        //This handles vibration updates. The observer updates the rumble values and this task updates in set intervals adjusting to those set values.
        //usb_task(time_us_64());

        //Only write to flash if a successful write block occurred. Set by a callback function
        //from DreamcastStorage. Pop blocking does what it says, blocks until something is ready
        //to be read. Since we don't want to stall this core, we first check to see if there is something
        //to read. If not, it goes about its way.
        if(multicore_fifo_rvalid())
        {
            uint32_t write_ack = multicore_fifo_pop_blocking();
            if(write_ack == 7) //TODO This is sloppy, clean it up
            {
                mem->process(); //Writes vmu storage to pico flash, this could probably be considered housekeeping
            }
        }
    }
}

// First Core Process
void core0()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Create the bus for client-mode operation
    std::shared_ptr<MapleBusInterface> bus = create_maple_bus(P1_BUS_START_PIN, P1_DIR_PIN, DIR_OUT_HIGH);

    // Main peripheral (address of 0x20) with 1 function: controller
    client::DreamcastMainPeripheral mainPeripheral(
        bus,
        0x20,
        0xFF,
        0x00,
        "Dreamcast Controller",
        "Version 1.010,1998/09/28,315-6211-AB   ,Analog Module : The 4th Edition.5/8  +DF",
        43.0,
        50.0);
    std::shared_ptr<client::DreamcastController> controller =
        std::make_shared<client::DreamcastController>();
    mainPeripheral.addFunction(controller);

    // First sub peripheral (address of 0x01) with 1 function: memory
    std::shared_ptr<client::DreamcastPeripheral> subPeripheral1 =
        std::make_shared<client::DreamcastPeripheral>(
            0x01,
            0xFF,
            0x00,
            "Visual Memory",
            "Version 1.005,1999/04/15,315-6208-03,SEGA Visual Memory System BIOS",
            12.4,
            13.0);
    std::shared_ptr<client::DreamcastStorage> dreamcastStorage =
        std::make_shared<client::DreamcastStorage>(mem, 0, storageCb);
    subPeripheral1->addFunction(dreamcastStorage);
    std::shared_ptr<client::DreamcastScreen> dreamcastScreen =
        std::make_shared<client::DreamcastScreen>(screenCb, 48, 32);
    subPeripheral1->addFunction(dreamcastScreen);
    Clock clock;
    std::shared_ptr<client::DreamcastTimer> dreamcastTimer =
        std::make_shared<client::DreamcastTimer>(clock, setTimeCb, setPwmFn);
    subPeripheral1->addFunction(dreamcastTimer);

    mainPeripheral.addSubPeripheral(subPeripheral1);

    // Second sub peripheral (address of 0x02) with 1 function: vibration
    /*std::shared_ptr<client::DreamcastPeripheral> subPeripheral2 =
        std::make_shared<client::DreamcastPeripheral>(
            0x02,
            0xFF,
            0x00,
            "Puru Puru Pack",
            "Version 1.000,1998/11/10,315-6211-AH   ,Vibration Motor:1 , Fm:4 - 30Hz ,Pow:7",
            20.0,
            160.0);
    std::shared_ptr<client::DreamcastVibration> dreamcastVibration =
        std::make_shared<client::DreamcastVibration>();
    dreamcastVibration->setObserver(get_usb_vibration_observer());
    subPeripheral2->addFunction(dreamcastVibration);
    mainPeripheral.addSubPeripheral(subPeripheral2);*/

    multicore_launch_core1(core1);

    while(true)
    {
        mainPeripheral.task(time_us_64());
        led_task(mem->getLastActivityTime());
    }
}

int main()
{
    led_init();

    gpio_init();

    core0();

    return 0;
}

#endif
