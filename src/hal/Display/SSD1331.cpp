#include "SSD1331.hpp"

namespace display
{
    SSD1331::SSD1331() :
        mDmaWriteChannel(dma_claim_unused_channel(true))
    {
        init();
    }

    void SSD1331::clear()
    {
        memset(mOledFB, 0, sizeof(mOledFB));
    }

    void SSD1331::write(const uint8_t data)
    {
        spi_write_blocking(SSD1331_SPI, &data, 1);
    }

    void SSD1331::update(const uint32_t* screen, uint32_t len)
    {
        gpio_put(DC, 0);

        write(0x15);
        write(0);
        write(95);
        write(0x75);
        write(0);
        write(63);

        gpio_put(DC, 1);

        dma_channel_transfer_from_buffer_now(mDmaWriteChannel, screen, len);
    }

    void SSD1331::splashScreen()
    {
        gpio_put(DC, 0);

        write(0x15);
        write(0);
        write(95);
        write(0x75);
        write(0);
        write(63);

        gpio_put(DC, 1);

        dma_channel_config c = dma_channel_get_default_config(mDmaWriteChannel);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, spi_get_index(SSD1331_SPI) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
        dma_channel_configure(mDmaWriteChannel, &c,
                                &spi_get_hw(SSD1331_SPI)->dr,          // write address
                                display::image_data_maplepad_logo_9664,         // read address
                                sizeof(display::image_data_maplepad_logo_9664), // element count (each element is of size transfer_data_size)
                                true);                                 // start

        // spi_write_blocking(SSD1331_SPI, oledFB, sizeof(oledFB));
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