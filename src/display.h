// #pragma once 

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#include "maple.h"
#include "menu.h"
#include "font.h"


extern tFont Font;

float cos_32s(float x);

float cos32(float x);

float sin32(float x);

double atan_66s(double x);

double atan66(double x);

void fast_hsv2rgb_32bit(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g , uint8_t *b);

void setPixel(uint8_t x, uint8_t y, uint16_t color);

void drawEllipse(int xc, int yc, int xr, int yr, int angle);

void drawLine(int x0, int y0, int w, uint16_t color);

void hagl_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void fillRect(int x0, int x1, int y0, int y1, uint16_t color);

void fillCircle(int x0, int y0, int r, uint16_t color);

void drawCursor(int iy, uint16_t color);

void drawToggle(int iy, uint16_t color, bool on);

void putLetter(int ix, int iy, int index, uint16_t color);

void putString(char* text, int ix, int iy, uint16_t color);

void updateDisplay(void);

void clearDisplay(void);

void displayInit(void);