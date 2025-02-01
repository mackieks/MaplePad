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

        void Display::drawToggle(int iy, uint16_t color, bool on)
    {
        if (on)
        {
            for (int i = 20; i >= 3; i--)
            {
                for (int j = 58; j >= 52; j--)
                {
                    if (!((i >= 4 && i <= 8) && (j >= 53 && j <= 57)))
                    {
                        setPixel(i, j - (12 * iy), color);
                    }
                }
            }
            setPixel(20, 58 - (12 * iy), 0x0000);
            setPixel(20, 57 - (12 * iy), 0x0000);
            setPixel(20, 53 - (12 * iy), 0x0000);
            setPixel(20, 52 - (12 * iy), 0x0000);
            setPixel(19, 58 - (12 * iy), 0x0000);
            setPixel(19, 52 - (12 * iy), 0x0000);

            setPixel(3, 58 - (12 * iy), 0x0000);
            setPixel(3, 57 - (12 * iy), 0x0000);
            setPixel(3, 53 - (12 * iy), 0x0000);
            setPixel(3, 52 - (12 * iy), 0x0000);
            setPixel(4, 58 - (12 * iy), 0x0000);
            setPixel(4, 52 - (12 * iy), 0x0000);

            setPixel(4, 57 - (12 * iy), color);
            setPixel(8, 57 - (12 * iy), color);
            setPixel(8, 53 - (12 * iy), color);
            setPixel(4, 53 - (12 * iy), color);
        } else {
            for (int i = 18; i >= 5; i--)
            {
                setPixel(i, 58 - (12 * iy), color);
                setPixel(i, 52 - (12 * iy), color);
            }
            for (int j = 56; j >= 54; j--)
            {
                setPixel(3, j - (12 * iy), color);
                setPixel(14, j - (12 * iy), color);
                setPixel(20, j - (12 * iy), color);
            }
            setPixel(4, 57 - (12 * iy), color);
            setPixel(4, 53 - (12 * iy), color);
            setPixel(15, 57 - (12 * iy), color);
            setPixel(15, 53 - (12 * iy), color);
            setPixel(19, 57 - (12 * iy), color);
            setPixel(19, 53 - (12 * iy), color);
        }
    }

    void Display::putLetter(int ix, int iy, char text, uint16_t color)
    {
        Font font;
        const uint8_t *a = font.getFontImage(text)->data;         // row of character data

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

    //text = "2   00:00  21%"; 14 chars
    void Display::showOverlay()
    {
        if (mIsOverlayRendered)
        {
            return;  // Skip if the overlay is already rendered
        }

        memset(oledFB+9216, {0x00}, 3072); //96x16x2
        
        char pageText[4];       // Buffer for page number ("1", "2", "3", .. "8")
        char timeText[9];       // Buffer for time ("00:00")
        char batteryText[6];    // Buffer for battery percentage ("21%")

        // Format the page number dynamically
        sprintf(pageText, "%d", mCurrentPage);

        if(mShowTimer)
        {
            sprintf(timeText, "00:00");  // hardcoded for now, replace with variable update via observer
        }
        else
        {
            sprintf(timeText, "     ");
        }

        if(mShowBatteryInd)
        {
            sprintf(batteryText, "100%%");  // hardcoded for now, replace with variable update via observer
        }
        else
        {
            sprintf(batteryText, "    ");
        }

        char finalText[20];  // Buffer to hold the final text
        snprintf(finalText, sizeof(finalText), "%-3s %-5s %-4s", pageText, timeText, batteryText);

        putString(finalText, 0, 0, 0xFFFF);

        mIsOverlayRendered = true;
    }

    void Display::notify(uint8_t& message)
    {
        mCurrentPage = message;
    }
}