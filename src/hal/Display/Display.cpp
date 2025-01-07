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

    void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
    {
        int16_t dx;
        int16_t sx;
        int16_t dy;
        int16_t sy;
        int16_t err;
        int16_t e2;

        dx = (x1 - x0);
        sx = x0 < x1 ? 1 : -1;
        dy = (y1 - y0);
        sy = y0 < y1 ? 1 : -1;
        err = (dx > dy ? dx : -dy) / 2;

        while (1) {
            setPixel(x0, y0, color);

            if (x0 == x1 && y0 == y1) {
                break;
            };

            e2 = err + err;

            if (e2 > -dx) {
                err -= dy;
                x0 += sx;
            }

            if (e2 < dy) {
                err += dx;
                y0 += sy;
            }
        }
    }

    void Display::drawRect(int x0, int x1, int y0, int y1, uint16_t color)
    {
        for (int i = y0; i <= y1; i++) {
            drawLine(x0, i, x1, i, color);
        }
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

    void Display::showOverlay()
    {
        clear();
        if(mCurrentPage == 1)
        {
            putString("VMU page 1", 0, 0, 0xFFFF);
        }else{
            putString("VMU page 2", 0, 0, 0xFFFF);
        }
        update();

        sleep_ms(50);
    }

    void Display::notify(uint8_t& message)
    {
        mCurrentPage = message;
    }
}