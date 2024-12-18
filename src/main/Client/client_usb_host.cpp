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
#include "SSD1331.hpp"
#include "Menu.hpp"

#include "led.hpp"

#include <memory>
#include <algorithm>
#include <cassert>

std::shared_ptr<display::Display> lcd;

void screenCb(const uint32_t* screen, uint32_t len)
{
    //len is the number of words in the payload. For this it should be 48 total words, or 192 bytes.
    //The bytes in each word of screen need to be reversed.
    if(lcd != nullptr && lcd->isInitialized())
    {
        lcd->refresh(screen, len);
    }
}

void setTimeCb(const client::DreamcastTimer::SetTime& setTime)
{
    // TODO: Fill in
}

void setPwmFn(uint8_t width, uint8_t down)
{
    if (width == 0 || down == 0)
    {
        //buzzer.stop(2);
    }
    else
    {
        //buzzer.stop(2);
        //buzzer.buzzRaw({.priority=2, .wrapCount=width, .highCount=(width-down)});
    }
}

void display_select()
{
    // OLED Select GPIO (high/open = SSD1331, Low = SSD1306)
    gpio_init(OLED_SEL_PIN);
    gpio_set_dir(OLED_SEL_PIN, false);
    gpio_pull_up(OLED_SEL_PIN);

    sleep_ms(50); // wait for pin to settle

    int oledType = gpio_get(OLED_SEL_PIN);
    switch(oledType)
    {
        case 0: //SSD1331
            break;
        case 1: //SSD1306
            lcd = std::make_shared<display::SSD1331>();
            break;
        default:
            break;
    }
}

//2MB of pico flash memory, 128KB of storage for VMU
std::shared_ptr<NonVolatilePicoSystemMemory> mem = std::make_shared<NonVolatilePicoSystemMemory>(
        PICO_FLASH_SIZE_BYTES - client::DreamcastStorage::MEMORY_SIZE_BYTES, //(2*1024*1024)=2097152-131072 = 1,966,080
        client::DreamcastStorage::MEMORY_SIZE_BYTES); //131,072;

// Second Core Process
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    while (true)
    {
        // Writes vmu storage to pico flash
        if(mem != nullptr)
        {
            mem->process();
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
        std::make_shared<client::DreamcastStorage>(mem, 0);
    subPeripheral1->addFunction(dreamcastStorage);

    //TODO add logic to check firmware version, format memory, and store it here
    //Format makes a call to store to flash so processing memory not necessary
    //dreamcastStorage->format();
    
    //Always start up on the first page
    //dreamcastStorage->updateSystemMemory(getMemoryBlockByPage(dreamcastStorage->updateCurrentPage(1)));

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

    //TODO uncomment once ready to work on persisting settings
    /*std::shared_ptr<NonVolatilePicoSystemMemory> flashData =
        std::make_shared<NonVolatilePicoSystemMemory>(
            PICO_FLASH_SIZE_BYTES - client::DreamcastStorage::MEMORY_SIZE_BYTES * 9, //Need to offset by the vmu size so we don't have collisions
            client::DreamcastStorage::FLASHDATA_SIZE_BYTES); //64
            */

    if(lcd != nullptr)
    {
        lcd->initialize();

        if(lcd->isInitialized())
        {
            if(controller->triggerMenu())
            {
                // Pass volatile memory pointer to flash data
                display::Menu menu(lcd);
                menu.run();
            }
            // Show splash after we exit the menu or if we don't enter the menu at all
            lcd->clear();
            lcd->showSplash();
        }
    }
    // Read flash data here for setting configurations. How should flash data be accessed by the controller?
    // Possibly via a controller update function?

    multicore_launch_core1(core1);

    while(true)
    {
        if(controller->triggerNextPage())
        {
            mem->nextPage(client::DreamcastStorage::MEMORY_SIZE_BYTES);
        }
        else if(controller->triggerPrevPage())
        {
            mem->prevPage(client::DreamcastStorage::MEMORY_SIZE_BYTES);
        }

        mainPeripheral.task(time_us_64());
        led_task(mem->getLastActivityTime());
    }
}

int main()
{
    stdio_init_all();
    
    led_init();

    display_select();

    core0();

    return 0;
}

#endif
