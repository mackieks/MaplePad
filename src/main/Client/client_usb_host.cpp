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
#include "SSD1331.hpp"

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
#include <cassert>

#define LCD_NumCols 6

display::SSD1331 lcd;
static uint8_t LCDFramebuffer[192] = {0};
volatile uint16_t palette[] = {
    0xf800, // red
    0xfba0, // orange
    0xff80, // yellow
    0x7f80, // yellow-green
    0x0500, // green
    0x045f, // blue
    0x781f, // violet
    0x780d  // magenta
};

// Function that reverses the byte order of a single uint32_t value
uint32_t reverseByteOrder(uint32_t value) {
    return ((value >> 24) & 0xFF) | 
           ((value >> 8) & 0xFF00) | 
           ((value << 8) & 0xFF0000) | 
           ((value << 24) & 0xFF000000);
}

void screenCb(const uint32_t* screen, uint32_t len)
{
    //len is in words, screen is a pointer to the data() in a uint32_t vector. This means
    //len is the number of words we should have available in the address screen points to.
    //If the ptr to screen here is empty, then that tells us there hasn't been any data
    //written to the maplebus for the screen. 48 words * 4 bytes per word should equal 192
    //total bytes that the VMU screen handles.
    if(*screen != 0 && (len * sizeof(uint32_t)) == sizeof(LCDFramebuffer))
    {
        uint32_t reversedArr[len];

        // Reverse the byte order of each element and store it in reversedArr
        for (size_t i = 0; i < len; ++i) {
            reversedArr[i] = reverseByteOrder(screen[i]);
        }

        memcpy(LCDFramebuffer, reversedArr, len * sizeof(uint32_t));

        for(int fb = 0; fb < 192; fb++)
        {
            for(int bb = 0; bb <= 7; bb++)
            {
                if(((LCDFramebuffer[fb] >> bb) & 0x01))
                {
                    lcd.setPixel(((fb % LCD_NumCols) * 8 + (7 - bb)) * 2, (fb / LCD_NumCols) * 2, palette[0]);
                    lcd.setPixel((((fb % LCD_NumCols) * 8 + (7 - bb)) * 2) + 1, (fb / LCD_NumCols) * 2, palette[0]);
                    lcd.setPixel(((fb % LCD_NumCols) * 8 + (7 - bb)) * 2, ((fb / LCD_NumCols) * 2) + 1, palette[0]);
                    lcd.setPixel((((fb % LCD_NumCols) * 8 + (7 - bb)) * 2) + 1, ((fb / LCD_NumCols) * 2) + 1, palette[0]);
                }
                else
                {
                    lcd.setPixel(((fb % LCD_NumCols) * 8 + (7 - bb)) * 2, (fb / LCD_NumCols) * 2, 0);
                    lcd.setPixel((((fb % LCD_NumCols) * 8 + (7 - bb)) * 2) + 1, (fb / LCD_NumCols) * 2, 0);
                    lcd.setPixel(((fb % LCD_NumCols) * 8 + (7 - bb)) * 2, ((fb / LCD_NumCols) * 2) + 1, 0);
                    lcd.setPixel((((fb % LCD_NumCols) * 8 + (7 - bb)) * 2) + 1, ((fb / LCD_NumCols) * 2) + 1, 0);
                }
            }
        }

        lcd.update();
        
        /*int x, y, pixel, bb;
        for (int fb = 0; fb < 192; fb++) {
            y = (fb / LCD_NumCols) * 2;
            int mod = (fb % LCD_NumCols) * 16;
            for (bb = 0; bb <= 7; bb++) {
                x = mod + (14 - bb * 2);
                pixel = ((LCDFramebuffer[fb] >> bb) & 0x01) * palette[0];
                lcd.setPixel(x, y, pixel);
                lcd.setPixel(x + 1, y, pixel);
                lcd.setPixel(x, y + 1, pixel);
                lcd.setPixel(x + 1, y + 1, pixel);
            }
        }
        lcd.update(screen, len);*/
    }
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
    /*if(multicore_fifo_wready())
    {
        
    }*/
    //multicore_fifo_push_blocking(state);
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

void gpio_init()
{
    // Configure ADC for triggers and stick
    adc_init();

    adc_set_clkdiv(0);
    adc_gpio_init(CTRL_PIN_SX); // Stick X
    adc_gpio_init(CTRL_PIN_SY); // Stick Y
    adc_gpio_init(CTRL_PIN_LT);  // Left Trigger
    adc_gpio_init(CTRL_PIN_RT);  // Right Trigger

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

    //Configure OLED SPI
    spi_init(SSD1331_SPI, SSD1331_SPEED);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(SCK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);
}

std::shared_ptr<NonVolatilePicoSystemMemory> mem =
    std::make_shared<NonVolatilePicoSystemMemory>(
        PICO_FLASH_SIZE_BYTES - client::DreamcastStorage::MEMORY_SIZE_BYTES,
        client::DreamcastStorage::MEMORY_SIZE_BYTES);

// Second Core Process
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    while (true)
    {
      // Writes vmu storage to pico flash
      mem->process();
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

    //TODO add logic to check firmware version, format memory, and store it here
    //Format makes a call to store to flash so processing memory not necessary
    //dreamcastStorage->format();

    std::shared_ptr<client::DreamcastScreen> dreamcastScreen =
        std::make_shared<client::DreamcastScreen>(screenCb, 48, 32);
    subPeripheral1->addFunction(dreamcastScreen);
    
    lcd.initialize();
    lcd.splashScreen();

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
    stdio_init_all();
    
    led_init();

    gpio_init();

    core0();

    return 0;
}

#endif
