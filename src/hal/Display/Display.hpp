#pragma once

#include "pico/stdlib.h"
#include <string.h>

namespace display
{

class Display
{
public:
    //! Virtual destructor
    virtual inline ~Display() {}

    //! Copies the pixel data into the LCD frame buffer used by the DMA channel
    void setPixel(uint8_t x, uint8_t y, uint16_t color);

    //! Refreshes the LCD panel by triggering a DMA transfer
    virtual void refresh(const uint32_t* screen, uint32_t len) = 0;

    //! Initializes the screen
    virtual void initialize() = 0;

    //! Clears the screen by setting the LCD frame buffer to black
    virtual void clear() = 0;

    //! Display a splash image
    virtual void showSplash() = 0;

    //! Returns true if the screen has been initialized, false otherwise
    inline bool isInitialized() { return mIsInitialized; }

    virtual void putString(char *text, int ix, int iy, uint16_t color) = 0;

    virtual void update() = 0;

    void drawCursor(int iy, uint16_t color);

    //! Function that reverses the byte order of a single uint32_t value
    inline uint32_t reverseByteOrder(uint32_t value) {
        return ((value >> 24) & 0xFF) | 
            ((value >> 8) & 0xFF00) | 
            ((value << 8) & 0xFF0000) | 
            ((value << 24) & 0xFF000000);
    }

public:
    //! Screen data block initialized to blank, 1536 bytes
    uint8_t oledFB[96 * 64 * 2] = {0x00};

protected:
    //! true iff "isInitialized"
    bool mIsInitialized = false;
};

}
