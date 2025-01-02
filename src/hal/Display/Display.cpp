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

    void Display::putLetter(int ix, int iy, char text, uint16_t color)
    {
        Font font;
        uint8_t *a = font.getFontImage(text)->data;         // row of character data

        for (int i = 0; i <= 9; i++) {
            for (int j = 2; j <= 7; j++) { // iterate through bits in row of character
            if (!((1 << j) & a[i]))
                setPixel((88 - (ix * 6)) - j, (59 - (iy * 10) - (iy * 2)) - i, color);
            else
                setPixel((88 - (ix * 6)) - j, (59 - (iy * 10) - (iy * 2)) - i, 0x0000);
            }
        }
    }

    void Display::putString(const char *text, int ix, int iy, uint16_t color)
    {
        for (int i = 0; text[i] != '\0'; i++)
        {
            putLetter(ix + i, iy, text[i], color);
        }
    }

    /*void Display::notify(uint8_t message)
    {
        mCurrentPage = message;//message.c_str();
    }*/
}