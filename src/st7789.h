#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define ST7789_SPI spi0
#define ST7789_SPEED 60000000 // 40MHz
#define ST7789_DC 5
#define ST7789_CS 1
#define ST7789_SCK 2
#define ST7789_MOSI 3
#define ST7789_RST 6
#define ST7789_BL_EN 7

#define ST7789_CMD_NOP             0x00        /**< no operation command */
#define ST7789_CMD_SWRESET         0x01        /**< software reset command */
#define ST7789_CMD_SLPIN           0x10        /**< sleep in command */
#define ST7789_CMD_SLPOUT          0x11        /**< sleep out command */
#define ST7789_CMD_PTLON           0x12        /**< partial mode on command */
#define ST7789_CMD_NORON           0x13        /**< normal display mode on command */
#define ST7789_CMD_INVOFF          0x20        /**< display inversion off command */
#define ST7789_CMD_INVON           0x21        /**< display inversion on command */
#define ST7789_CMD_GAMSET          0x26        /**< display inversion set command */
#define ST7789_CMD_DISPOFF         0x28        /**< display off command */
#define ST7789_CMD_DISPON          0x29        /**< display on command */
#define ST7789_CMD_CASET           0x2A        /**< column address set command */
#define ST7789_CMD_RASET           0x2B        /**< row address set command */
#define ST7789_CMD_RAMWR           0x2C        /**< memory write command */
#define ST7789_CMD_PTLAR           0x30        /**< partial start/end address set command */
#define ST7789_CMD_VSCRDEF         0x33        /**< vertical scrolling definition command */
#define ST7789_CMD_TEOFF           0x34        /**< tearing effect line off command */
#define ST7789_CMD_TEON            0x35        /**< tearing effect line on command */
#define ST7789_CMD_MADCTL          0x36        /**< memory data access control command */
#define ST7789_CMD_VSCRSADD        0x37        /**< vertical scrolling start address command */
#define ST7789_CMD_IDMOFF          0x38        /**< idle mode off command */
#define ST7789_CMD_IDMON           0x39        /**< idle mode on command */
#define ST7789_CMD_COLMOD          0x3A        /**< interface pixel format command */
#define ST7789_CMD_RAMWRC          0x3C        /**< memory write continue command */
#define ST7789_CMD_TESCAN          0x44        /**< set tear scanline command */
#define ST7789_CMD_WRDISBV         0x51        /**< write display brightness command */
#define ST7789_CMD_WRCTRLD         0x53        /**< write CTRL display command */
#define ST7789_CMD_WRCACE          0x55        /**< write content adaptive brightness control and color enhancement command */
#define ST7789_CMD_WRCABCMB        0x5E        /**< write CABC minimum brightness command */
#define ST7789_CMD_RAMCTRL         0xB0        /**< ram control command */
#define ST7789_CMD_RGBCTRL         0xB1        /**< rgb control command */
#define ST7789_CMD_PORCTRL         0xB2        /**< porch control command */
#define ST7789_CMD_FRCTRL1         0xB3        /**< frame rate control 1 command */
#define ST7789_CMD_PARCTRL         0xB5        /**< partial mode control command */
#define ST7789_CMD_GCTRL           0xB7        /**< gate control command */
#define ST7789_CMD_GTADJ           0xB8        /**< gate on timing adjustment command */
#define ST7789_CMD_DGMEN           0xBA        /**< digital gamma enable command */
#define ST7789_CMD_VCOMS           0xBB        /**< vcoms setting command */
#define ST7789_CMD_LCMCTRL         0xC0        /**< lcm control command */
#define ST7789_CMD_IDSET           0xC1        /**< id setting command */
#define ST7789_CMD_VDVVRHEN        0xC2        /**< vdv and vrh command enable command */
#define ST7789_CMD_VRHS            0xC3        /**< vrh set command */
#define ST7789_CMD_VDVSET          0xC4        /**< vdv setting command */
#define ST7789_CMD_VCMOFSET        0xC5        /**< vcoms offset set command */
#define ST7789_CMD_FRCTR2          0xC6        /**< fr control 2 command */
#define ST7789_CMD_CABCCTRL        0xC7        /**< cabc control command */
#define ST7789_CMD_REGSEL1         0xC8        /**< register value selection1 command */
#define ST7789_CMD_REGSEL2         0xCA        /**< register value selection2 command */
#define ST7789_CMD_PWMFRSEL        0xCC        /**< pwm frequency selection command */
#define ST7789_CMD_PWCTRL1         0xD0        /**< power control 1 command */
#define ST7789_CMD_VAPVANEN        0xD2        /**< enable vap/van signal output command */
#define ST7789_CMD_CMD2EN          0xDF        /**< command 2 enable command */
#define ST7789_CMD_PVGAMCTRL       0xE0        /**< positive voltage gamma control command */
#define ST7789_CMD_NVGAMCTRL       0xE1        /**< negative voltage gamma control command */
#define ST7789_CMD_DGMLUTR         0xE2        /**< digital gamma look-up table for red command */
#define ST7789_CMD_DGMLUTB         0xE3        /**< digital gamma look-up table for blue command */
#define ST7789_CMD_GATECTRL        0xE4        /**< gate control command */
#define ST7789_CMD_SPI2EN          0xE7        /**< spi2 command */
#define ST7789_CMD_PWCTRL2         0xE8        /**< power control 2 command */
#define ST7789_CMD_EQCTRL          0xE9        /**< equalize time control command */
#define ST7789_CMD_PROMCTRL        0xEC        /**< program control command */
#define ST7789_CMD_PROMEN          0xFA        /**< program mode enable command */
#define ST7789_CMD_NVMSET          0xFC        /**< nvm setting command */
#define ST7789_CMD_PROMACT         0xFE        /**< program action command */

void st7789WriteCommand(const uint8_t data);

void st7789WriteData(const uint8_t data);

void st7789SetPixel(const uint8_t x, const uint8_t y, const uint16_t color);

void st7789_clear(void);

void st7789_update(void);

void st7789_splash(void);

void st7789_init();