#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define SH8601_SPI spi0
#define SH8601_SPEED 20000000 // 62.5MHz
#define SH8601_DC 0
#define SH8601_CS 1
#define SH8601_SCK 2
#define SH8601_MOSI 3
#define SH8601_RST 5
#define SH8601_PWR_EN 6

#define SH8601_OLED_W 368
#define SH8601_OLED_H 448

#define OLED_FLIP flashData[18]

// SH8601 Commands
#define SH8601_CMD_NOP 0x00
#define SH8601_CMD_SWRESET 0x01
#define SH8601_READ_ID 0x04
#define SH8601_READ_DSI_ERR 0x05
#define SH8601_READ_PWRMODE 0x0A
#define SH8601_READ_MADCTL 0x0B
#define SH8601_READ_PXLMODE 0x0C
#define SH8601_READ_IMGMODE 0x0D
#define SH8601_READ_SGNLMODE 0x0E
#define SH8601_READ_SELFDIAG 0x0F
#define SH8601_SLEEPIN 0x10
#define SH8601_SLEEPOUT 0x11
#define SH8601_PARTIAL_DISP_MODE_ON 0x12
#define SH8601_NORMAL_DISP_MODE_ON 0x13
#define SH8601_INVERT_OFF 0x20
#define SH8601_INVERT_ON 0x21
#define SH8601_ALL_PXL_OFF 0x22
#define SH8601_ALL_PXL_ON 0x23
#define SH8601_DISP_OFF 0x28
#define SH8601_DISP_ON 0x29
#define SH8601_COLSET 0x2A
#define SH8601_PGADDRSET 0x2B
#define SH8601_MEMWRITE 0x2C
#define SH8601_PARTIAL_ROWSET 0x30
#define SH8601_PARTIAL_COLSET 0x31
#define SH8601_TE_OFF 0x34
#define SH8601_TE_ON 0x35
#define SH8601_MADCTL 0x36
#define SH8601_IDLE_OFF 0x38
#define SH8601_IDLE_ON 0x39
#define SH8601_WRITE_PXL_FMT 0x3A
#define SH8601_MEMWRITE_CONT 0x3C
#define SH8601_WRITE_TE 0x44
#define SH8601_READ_SCANLINE_NO 0x45
#define SH8601_SPI_RD_OFF 0x46
#define SH8601_SPI_RD_ON 0x47
#define SH8601_AOD_OFF 0x48
#define SH8601_AOD_ON 0x49
#define SH8601_WRITE_AODBRT 0x4A
#define SH8601_READ_AODBRT 0x4B
#define SH8601_DEEP_STDBY 0x4F
#define SH8601_WRITE_BRT 0x51
#define SH8601_READ_BRT 0x52
#define SH8601_WRITE_CTRL1 0x53
#define SH8601_READ_CTRL1 0x54
#define SH8601_WRITE_CTRL2 0x55
#define SH8601_READ_CTRL2 0x56
#define SH8601_WRITE_CE 0x58
#define SH8601_READ_CE 0x59
#define SH8601_WRITE_HBMBRT 0x63
#define SH8601_READ_HBMBRT 0x64
#define SH8601_WRITE_HBMCTRL 0x66
#define SH8601_READ_DDB_ST 0xA1
#define SH8601_READ_DDB_CT 0xA8
#define SH8601_READ_CRC_ST 0xAA
#define SH8601_READ_CRC_CT 0xAF
#define SH8601_SPI_MODE 0xC4
#define SH8601_READ_ID1 0xDA
#define SH8601_READ_ID2 0xDB
#define SH8601_READ_ID3 0xDC

void sh8601WriteCommand(const uint8_t data);

void sh8601WriteData(const uint8_t data);

void sh8601SetPixel(const uint8_t x, const uint8_t y, const uint16_t color);

void sh8601_clear(void);

void sh8601_update(void);

void sh8601_splash(void);

void sh8601_init();