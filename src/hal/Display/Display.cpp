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

    void Display::putLetter(int ix, int iy, int index, uint16_t color)
    {
        tChar y = Font_array[index]; // select character from 0 - 54
        tImage z = y.image;         // data array from tImage "Font_0x79"
        const uint8_t *a = z.data;         // row of character data

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
            switch (text[i])
            {
                case ' ':
                    putLetter(ix + i, iy, 0, color);
                break;

                case '-':
                    putLetter(ix + i, iy, 1, color);
                break;

                case '.':
                    putLetter(ix + i, iy, 2, color);
                break;

                case 'A':
                    putLetter(ix + i, iy, 3, color);
                break;

                case 'B':
                    putLetter(ix + i, iy, 4, color);
                break;

                case 'C':
                    putLetter(ix + i, iy, 5, color);
                break;

                case 'D':
                    putLetter(ix + i, iy, 6, color);
                break;

                case 'E':
                    putLetter(ix + i, iy, 7, color);
                break;

                case 'F':
                    putLetter(ix + i, iy, 8, color);
                break;

                case 'G':
                    putLetter(ix + i, iy, 9, color);
                break;

                case 'H':
                    putLetter(ix + i, iy, 10, color);
                break;

                case 'I':
                    putLetter(ix + i, iy, 11, color);
                break;

                case 'J':
                    putLetter(ix + i, iy, 12, color);
                break;

                case 'K':
                    putLetter(ix + i, iy, 13, color);
                break;

                case 'L':
                    putLetter(ix + i, iy, 14, color);
                break;

                case 'M':
                    putLetter(ix + i, iy, 15, color);
                break;

                case 'N':
                    putLetter(ix + i, iy, 16, color);
                break;

                case 'O':
                    putLetter(ix + i, iy, 17, color);
                break;

                case 'P':
                    putLetter(ix + i, iy, 18, color);
                break;

                case 'Q':
                    putLetter(ix + i, iy, 19, color);
                break;

                case 'R':
                    putLetter(ix + i, iy, 20, color);
                break;

                case 'S':
                    putLetter(ix + i, iy, 21, color);
                break;

                case 'T':
                    putLetter(ix + i, iy, 22, color);
                break;

                case 'U':
                    putLetter(ix + i, iy, 23, color);
                break;

                case 'V':
                    putLetter(ix + i, iy, 24, color);
                break;

                case 'W':
                    putLetter(ix + i, iy, 25, color);
                break;

                case 'X':
                    putLetter(ix + i, iy, 26, color);
                break;

                case 'Y':
                    putLetter(ix + i, iy, 27, color);
                break;

                case 'Z':
                    putLetter(ix + i, iy, 28, color);
                break;

                case 'a':
                    putLetter(ix + i, iy, 29, color);
                break;

                case 'b':
                    putLetter(ix + i, iy, 30, color);
                break;

                case 'c':
                    putLetter(ix + i, iy, 31, color);
                break;

                case 'd':
                    putLetter(ix + i, iy, 32, color);
                break;

                case 'e':
                    putLetter(ix + i, iy, 33, color);
                break;

                case 'f':
                    putLetter(ix + i, iy, 34, color);
                break;

                case 'g':
                    putLetter(ix + i, iy, 35, color);
                break;

                case 'h':
                    putLetter(ix + i, iy, 36, color);
                break;

                case 'i':
                    putLetter(ix + i, iy, 37, color);
                break;

                case 'j':
                    putLetter(ix + i, iy, 38, color);
                break;

                case 'k':
                    putLetter(ix + i, iy, 39, color);
                break;

                case 'l':
                    putLetter(ix + i, iy, 40, color);
                break;

                case 'm':
                    putLetter(ix + i, iy, 41, color);
                break;

                case 'n':
                    putLetter(ix + i, iy, 42, color);
                break;

                case 'o':
                    putLetter(ix + i, iy, 43, color);
                break;

                case 'p':
                    putLetter(ix + i, iy, 44, color);
                break;

                case 'q':
                    putLetter(ix + i, iy, 45, color);
                break;

                case 'r':
                    putLetter(ix + i, iy, 46, color);
                break;

                case 's':
                    putLetter(ix + i, iy, 47, color);
                break;

                case 't':
                    putLetter(ix + i, iy, 48, color);
                break;

                case 'u':
                    putLetter(ix + i, iy, 49, color);
                break;

                case 'v':
                    putLetter(ix + i, iy, 50, color);
                break;

                case 'w':
                    putLetter(ix + i, iy, 51, color);
                break;

                case 'x':
                    putLetter(ix + i, iy, 52, color);
                break;

                case 'y':
                    putLetter(ix + i, iy, 53, color);
                break;

                case 'z':
                    putLetter(ix + i, iy, 54, color);
                break;

                case '!':
                    putLetter(ix + i, iy, 55, color);
                break;

                case '#':
                    putLetter(ix + i, iy, 56, color);
                break;

                case '%':
                    putLetter(ix + i, iy, 57, color);
                break;

                case '&':
                    putLetter(ix + i, iy, 58, color);
                break;

                case '\'':
                    putLetter(ix + i, iy, 59, color);
                break;

                case '(':
                    putLetter(ix + i, iy, 60, color);
                break;

                case ')':
                    putLetter(ix + i, iy, 61, color);
                break;

                case '*':
                    putLetter(ix + i, iy, 62, color);
                break;

                case '+':
                    putLetter(ix + i, iy, 63, color);
                break;

                case ',':
                    putLetter(ix + i, iy, 64, color);
                break;

                case '0':
                    putLetter(ix + i, iy, 65, color);
                break;

                case '1':
                    putLetter(ix + i, iy, 66, color);
                break;

                case '2':
                    putLetter(ix + i, iy, 67, color);
                break;

                case '3':
                    putLetter(ix + i, iy, 68, color);
                break;

                case '4':
                    putLetter(ix + i, iy, 69, color);
                break;

                case '5':
                    putLetter(ix + i, iy, 70, color);
                break;

                case '6':
                    putLetter(ix + i, iy, 71, color);
                break;

                case '7':
                    putLetter(ix + i, iy, 72, color);
                break;

                case '8':
                    putLetter(ix + i, iy, 73, color);
                break;

                case '9':
                    putLetter(ix + i, iy, 74, color);
                break;

                case ':':
                    putLetter(ix + i, iy, 75, color);
                break;

                case ';':
                    putLetter(ix + i, iy, 76, color);
                break;

                case '=':
                    putLetter(ix + i, iy, 77, color);
                break;

                default:
                break;
            }
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

        // Format the page number dynamically (1 or 2)
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