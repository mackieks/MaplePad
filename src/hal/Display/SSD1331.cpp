#include "SSD1331.hpp"

namespace display
{
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

    static uint8_t LCDFramebuffer[192] = {0};
    
    SSD1331::SSD1331() :
        mDmaWriteChannel(dma_claim_unused_channel(true)),
        mConfig(dma_channel_get_default_config(mDmaWriteChannel))
    {
        mIsInitialized = false;
    }

    void SSD1331::clear()
    {
        memset(mOledFB, 0, sizeof(mOledFB));
    }

    void SSD1331::write(const uint8_t data)
    {
        spi_write_blocking(SSD1331_SPI, &data, 1);
    }

    //! Refreshes the screen of the SSD1331
    //! In order to prevent issues with pushing an all empty array to the screen,
    //! a check is made to ensure that there is something to write before continuing.
    //! Since the screen array is already being iterated in this method, it made the most
    //! sense to include that check here.
    void SSD1331::refresh(const uint32_t* screen, uint32_t len)
    {
        uint32_t reversedArr[len];

        // Counter for the amount of words that are empty
        uint32_t zeroWords = 0;
        // Reverse the byte order of each element and store it in reversedArr
        for (size_t i = 0; i < len; ++i) {
            reversedArr[i] = reverseByteOrder(screen[i]);
            // If the word is 0 or 'empty', increase count
            if(reversedArr[i] == 0)
            {
                zeroWords++;
            }
        }

        // If all the words are zero, we don't want to write anything to the screen so we return
        if(zeroWords == len)
        {
            return;
        }

        memcpy(LCDFramebuffer, reversedArr, len * sizeof(uint32_t));

        int x, y, pixel, bb;
        for (int fb = 0; fb < 192; fb++) {
            y = (fb / 6) * 2;
            int mod = (fb % 6) * 16;
            for (bb = 0; bb <= 7; bb++) {
                x = mod + (14 - bb * 2);
                pixel = ((LCDFramebuffer[fb] >> bb) & 0x01) * palette[0];
                setPixel(x, y, pixel);
                setPixel(x + 1, y, pixel);
                setPixel(x, y + 1, pixel);
                setPixel(x + 1, y + 1, pixel);
            }
        }

        gpio_put(DC, 0);

        write(0x15);
        write(0);
        write(95);
        write(0x75);
        write(0);
        write(63);

        gpio_put(DC, 1);

        if (!(dma_channel_is_busy(mDmaWriteChannel)))
        {
            dma_channel_configure(mDmaWriteChannel, &mConfig,
                                    &spi_get_hw(SSD1331_SPI)->dr,          // write address
                                    mOledFB,         // read address
                                    sizeof(mOledFB), // element count (each element is of size transfer_data_size)
                                    true);                                 // start
        }
    }

    void SSD1331::setPixel(uint8_t x, uint8_t y, uint16_t color)
    {
        // Set Pixel
        // uint8_t r = (color & 0xF800) >> 11;
        // uint8_t g = (color & 0x7E0) >> 5;
        // uint8_t b = (color & 0x1f);

        memset(&mOledFB[(y * 192) + (x * 2)], color >> 8, sizeof(uint8_t));
        memset(&mOledFB[(y * 192) + (x * 2) + 1], color & 0xff, sizeof(uint8_t));
    }

    void SSD1331::showSplash()
    {
        gpio_put(DC, 0);

        write(0x15);
        write(0);
        write(95);
        write(0x75);
        write(0);
        write(63);

        gpio_put(DC, 1);

        //memcpy(mOledFB, display::image_data_maplepad_logo_9664, sizeof(display::image_data_maplepad_logo_9664));

        if (!(dma_channel_is_busy(mDmaWriteChannel)))
        {
            dma_channel_configure(mDmaWriteChannel, &mConfig,
                                    &spi_get_hw(SSD1331_SPI)->dr,          // write address
                                    display::image_data_maplepad_logo_9664,         // read address
                                    sizeof(display::image_data_maplepad_logo_9664), // element count (each element is of size transfer_data_size)
                                    true);                                 // start
        }

        // spi_write_blocking(SSD1331_SPI, oledFB, sizeof(oledFB));
    }

    void SSD1331::putLetter(int ix, int iy, int index, uint16_t color)
    {
        Font font;
        tChar y = font.getFont().chars[index]; // select character from 0 - 54
        tImage z = *y.image;         // data array from tImage "Font_0x79"
        uint8_t *a = z.data;         // row of character data

        for (int i = 0; i <= 9; i++) {
            for (int j = 2; j <= 7; j++) { // iterate through bits in row of character
            if (!((1 << j) & a[i]))
                setPixel((88 - (ix * 6)) - j, (59 - (iy * 10) - (iy * 2)) - i, color);
            else
                setPixel((88 - (ix * 6)) - j, (59 - (iy * 10) - (iy * 2)) - i, 0x0000);
            }
        }
    }

    void SSD1331::initialize()
    {
        //Configure OLED SPI
        spi_init(SSD1331_SPI, SSD1331_SPEED);
        spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        gpio_set_function(SCK, GPIO_FUNC_SPI);
        gpio_set_function(MOSI, GPIO_FUNC_SPI);

        init();

        channel_config_set_transfer_data_size(&mConfig, DMA_SIZE_8);
        channel_config_set_dreq(&mConfig, spi_get_index(SSD1331_SPI) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
        mIsInitialized = true;
    }

    void SSD1331::init()
    {
        gpio_init(DC);
        gpio_set_dir(DC, GPIO_OUT);
        gpio_put(DC, 1);
        gpio_init(RST);
        gpio_set_dir(RST, GPIO_OUT);
        gpio_put(RST, 0);
        sleep_ms(50);
        gpio_put(RST, 1);
        gpio_put(DC, 0);
        // Initialization Sequence
        write(SSD1331_CMD_DISPLAYOFF); // 0xAE
        write(SSD1331_CMD_SETREMAP);   // 0xA0

        //if (OLED_FLIP)
        write(0x60); //Start with flipped OLED for now
        //else
            //write(0x72);

        write(SSD1331_CMD_STARTLINE); // 0xA1
        write(0x0);
        write(SSD1331_CMD_DISPLAYOFFSET); // 0xA2
        write(0x0);
        write(SSD1331_CMD_NORMALDISPLAY); // 0xA4
        write(SSD1331_CMD_SETMULTIPLEX);  // 0xA8
        write(0x3F);                      // 0x3F 1/64 duty
        write(SSD1331_CMD_SETMASTER);     // 0xAD
        write(0x8E);
        write(SSD1331_CMD_POWERMODE); // 0xB0
        write(0x0B);
        write(SSD1331_CMD_PRECHARGE); // 0xB1
        write(0x31);
        write(SSD1331_CMD_CLOCKDIV);   // 0xB3
        write(0xF0);                   // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio
                                                    // (A[3:0]+1 = 1..16)
        write(SSD1331_CMD_PRECHARGEA); // 0x8A
        write(0x64);
        write(SSD1331_CMD_PRECHARGEB); // 0x8B
        write(0x78);
        write(SSD1331_CMD_PRECHARGEC); // 0x8C
        write(0x64);
        write(SSD1331_CMD_PRECHARGELEVEL); // 0xBB
        write(0x3A);
        write(SSD1331_CMD_VCOMH); // 0xBE
        write(0x3E);
        write(SSD1331_CMD_MASTERCURRENT); // 0x87
        write(0x0f);
        write(SSD1331_CMD_CONTRASTA); // 0x81
        write(0xFF);
        write(SSD1331_CMD_CONTRASTB); // 0x82
        write(0xFF);
        write(SSD1331_CMD_CONTRASTC); // 0x83
        write(0xFF);
        write(SSD1331_CMD_DISPLAYALLOFF);
        write(SSD1331_CMD_NORMALDISPLAY); // 0xA4
        write(SSD1331_CMD_DISPLAYON);     //--turn on oled panel
    }
}