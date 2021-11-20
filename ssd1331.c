#include "ssd1331.h"
#include <string.h>

uint8_t oledFB[96 * 64 * 2] = {0x00};

void ssd1331WriteCommand(const uint8_t data){
    spi_write_blocking(SSD1331_SPI, &data, 1);
}

void ssd1331WriteCommands(const uint8_t* data, uint num){
    spi_write_blocking(SSD1331_SPI, data, num);
}

void ssd1331WriteData(const uint8_t* data, uint numbytes){
    // Write to register
    gpio_put(DC, 1);
    spi_write_blocking(SSD1331_SPI, data, numbytes);
}

void setPixelSSD1331(const uint8_t x, const uint8_t y, const uint16_t color){
  // Set Pixel
  uint8_t r = (color >> 10) & 0x3e;
  uint8_t g = (color >> 5) & 0x3e;
  uint8_t b = (color & 0x1f) << 1;

  ssd1331WriteCommand(0x21);
  ssd1331WriteCommand(x);
  ssd1331WriteCommand(x);
  ssd1331WriteCommand(y);
  ssd1331WriteCommand(y);
  ssd1331WriteCommand(r);
  ssd1331WriteCommand(g);
  ssd1331WriteCommand(b);
}

void clearSSD1331(void){
  uint8_t fb[96*64*2] = {0x00};
  spi_write_blocking(SSD1331_SPI, fb, sizeof(fb));
}

void ssd1331_init() {
  gpio_init(DC);
  gpio_set_dir(DC, GPIO_OUT);
  gpio_put(DC, 1);
  gpio_init(RST);
  gpio_put(RST, 0);
  sleep_ms(50);
  gpio_put(RST, 1);
  gpio_put(DC, 0);
  // Initialization Sequence
  ssd1331WriteCommand(SSD1331_CMD_DISPLAYOFF); // 0xAE
  ssd1331WriteCommand(SSD1331_CMD_SETREMAP);   // 0xA0
#if defined SSD1331_COLORORDER_RGB
  ssd1331WriteCommand(0x72); // RGB Color
#else
  ssd1331WriteCommand(0x76); // BGR Color
#endif
  ssd1331WriteCommand(SSD1331_CMD_STARTLINE); // 0xA1
  ssd1331WriteCommand(0x0);
  ssd1331WriteCommand(SSD1331_CMD_DISPLAYOFFSET); // 0xA2
  ssd1331WriteCommand(0x0);
  ssd1331WriteCommand(SSD1331_CMD_NORMALDISPLAY); // 0xA4
  ssd1331WriteCommand(SSD1331_CMD_SETMULTIPLEX);  // 0xA8
  ssd1331WriteCommand(0x3F);                      // 0x3F 1/64 duty
  ssd1331WriteCommand(SSD1331_CMD_SETMASTER);     // 0xAD
  ssd1331WriteCommand(0x8E);
  ssd1331WriteCommand(SSD1331_CMD_POWERMODE); // 0xB0
  ssd1331WriteCommand(0x0B);
  ssd1331WriteCommand(SSD1331_CMD_PRECHARGE); // 0xB1
  ssd1331WriteCommand(0x31);
  ssd1331WriteCommand(SSD1331_CMD_CLOCKDIV); // 0xB3
  ssd1331WriteCommand(0xF0); // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio
                     // (A[3:0]+1 = 1..16)
  ssd1331WriteCommand(SSD1331_CMD_PRECHARGEA); // 0x8A
  ssd1331WriteCommand(0x64);
  ssd1331WriteCommand(SSD1331_CMD_PRECHARGEB); // 0x8B
  ssd1331WriteCommand(0x78);
  ssd1331WriteCommand(SSD1331_CMD_PRECHARGEC); // 0x8C
  ssd1331WriteCommand(0x64);
  ssd1331WriteCommand(SSD1331_CMD_PRECHARGELEVEL); // 0xBB
  ssd1331WriteCommand(0x3A);
  ssd1331WriteCommand(SSD1331_CMD_VCOMH); // 0xBE
  ssd1331WriteCommand(0x3E);
  ssd1331WriteCommand(SSD1331_CMD_MASTERCURRENT); // 0x87
  ssd1331WriteCommand(0x06);
  ssd1331WriteCommand(SSD1331_CMD_CONTRASTA); // 0x81
  ssd1331WriteCommand(0x91);
  ssd1331WriteCommand(SSD1331_CMD_CONTRASTB); // 0x82
  ssd1331WriteCommand(0x50);
  ssd1331WriteCommand(SSD1331_CMD_CONTRASTC); // 0x83
  ssd1331WriteCommand(0x7D);
  ssd1331WriteCommand(SSD1331_CMD_DISPLAYALLOFF);
  ssd1331WriteCommand(SSD1331_CMD_NORMALDISPLAY); // 0xA4
  ssd1331WriteCommand(SSD1331_CMD_DISPLAYON); //--turn on oled panel
}