#include "Display.hpp"

namespace display
{
    void Display::drawCursor(int iy, uint16_t color)
    {
        for (int i = 87; i <= 95; i++)
        {
            for (int j = 0; j <= 63; j++)
            {
                setPixel(i, j, 0x00);
            }
        }

        for (int i = 93; i >= 90; i--)
        {
            for (int j = 57; j >= 53; j--)
            {
                if (i > 90)
                {
                    setPixel(i, j - (12 * iy), color);
                }
                else if ((j > 53) && (j < 57))
                {
                    setPixel(i, j - (12 * iy), color);
                }
            }
        }
        setPixel(89, 55 - (12 * iy), color);
    }

    void Display::setPixel(uint8_t x, uint8_t y, uint16_t color)
    {
        memset(&oledFB[(y * 192) + (x * 2)], color >> 8, sizeof(uint8_t));
        memset(&oledFB[(y * 192) + (x * 2) + 1], color & 0xff, sizeof(uint8_t));
    }
}