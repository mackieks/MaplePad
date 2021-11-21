#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#define SSD1331_SPI spi0
#define SSD1331_SPEED  62500000
#define SCK 2
#define MOSI 3
#define DC 14
#define RST 13

#define OLED_W 96
#define OLED_H 64

// SSD1331 Commands
#define SSD1331_CMD_DRAWLINE 0x21      //!< Draw line
#define SSD1331_CMD_DRAWRECT 0x22      //!< Draw rectangle
#define SSD1331_CMD_FILL 0x26          //!< Fill enable/disable
#define SSD1331_CMD_SETCOLUMN 0x15     //!< Set column address
#define SSD1331_CMD_SETROW 0x75        //!< Set row adress
#define SSD1331_CMD_CONTRASTA 0x81     //!< Set contrast for color A
#define SSD1331_CMD_CONTRASTB 0x82     //!< Set contrast for color B
#define SSD1331_CMD_CONTRASTC 0x83     //!< Set contrast for color C
#define SSD1331_CMD_MASTERCURRENT 0x87 //!< Master current control
#define SSD1331_CMD_SETREMAP 0xA0      //!< Set re-map & data format
#define SSD1331_CMD_STARTLINE 0xA1     //!< Set display start line
#define SSD1331_CMD_DISPLAYOFFSET 0xA2 //!< Set display offset
#define SSD1331_CMD_NORMALDISPLAY 0xA4 //!< Set display to normal mode
#define SSD1331_CMD_DISPLAYALLON 0xA5  //!< Set entire display ON
#define SSD1331_CMD_DISPLAYALLOFF 0xA6 //!< Set entire display OFF
#define SSD1331_CMD_INVERTDISPLAY 0xA7 //!< Invert display
#define SSD1331_CMD_SETMULTIPLEX 0xA8  //!< Set multiplex ratio
#define SSD1331_CMD_SETMASTER 0xAD     //!< Set master configuration
#define SSD1331_CMD_DISPLAYOFF 0xAE    //!< Display OFF (sleep mode)
#define SSD1331_CMD_DISPLAYON 0xAF     //!< Normal Brightness Display ON
#define SSD1331_CMD_POWERMODE 0xB0     //!< Power save mode
#define SSD1331_CMD_PRECHARGE 0xB1     //!< Phase 1 and 2 period adjustment
#define SSD1331_CMD_CLOCKDIV   0xB3 //!< Set display clock divide ratio/oscillator frequency
#define SSD1331_CMD_PRECHARGEA 0x8A //!< Set second pre-charge speed for color A
#define SSD1331_CMD_PRECHARGEB 0x8B //!< Set second pre-charge speed for color B
#define SSD1331_CMD_PRECHARGEC 0x8C //!< Set second pre-charge speed for color C
#define SSD1331_CMD_PRECHARGELEVEL 0xBB //!< Set pre-charge voltage
#define SSD1331_CMD_VCOMH 0xBE          //!< Set Vcomh voltge


void ssd1331WriteCommand(const uint8_t data);

void ssd1331WriteCommands(const uint8_t* data, uint num);

void ssd1331WriteData(const uint8_t* data, uint numbytes);

void setPixelSSD1331(const uint8_t x, const uint8_t y, const uint16_t color);

void clearSSD1331(void);

void updateSSD1331(void);

void ssd1331_init();