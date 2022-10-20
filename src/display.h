#pragma once 

#include <stdint.h>
#include "pico/stdlib.h"
#include <string.h>
#include <math.h>
#include "font.h"
#include "ssd1331.h"
#include "ssd1306.h"

float cos_32s(float x);

float cos32(float x);

float sin32(float x);

double atan_66s(double x);

double atan66(double x);

void fast_hsv2rgb_32bit(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g , uint8_t *b);

void setPixel(uint8_t x, uint8_t y, uint16_t color, uint8_t colorOLED);

void drawEllipse(int xc, int yc, int xr, int yr, int angle);

void drawLine(int x0, int y0, int w, uint16_t color);

void hagl_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void fillRect(int x0, int x1, int y0, int y1, uint16_t color);

void fillCircle(int x0, int y0, int r, uint16_t color);

void drawCursor(int iy, uint16_t color);

void drawToggle(int iy, uint16_t color, bool on);

void putLetter(int ix, int iy, int index, uint16_t color);

void putString(char* text, int ix, int iy, uint16_t color);

void updateDisplay(uint8_t oledType);

void clearDisplay(uint8_t oledType);

void displayInit(uint8_t oledType);