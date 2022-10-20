#pragma once

#include <stdint.h>

 typedef struct {
     uint8_t *data;
     uint16_t width;
     uint16_t height;
     uint8_t dataSize;
     } tImage;
 typedef struct {
     long int code;
     tImage *image;
     } tChar;
 typedef struct {
     int length;
     tChar *chars;
     } tFont;
