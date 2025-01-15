#include "Menu.hpp"

namespace display
{

    Menu::Menu(std::shared_ptr<Display> lcd) :
        mCurrentNumEntries(sizeof(mainMenu) / sizeof(MenuItem)),
        mCurrentMenu(mainMenu),
        mDisplay(lcd)
    {
        enterMainMenu(NULL);
    }

    int Menu::exitToPad(MenuItem *self)
    {
        return 0;
    }

    int Menu::enterMainMenu(MenuItem *self)
    {
        mainMenu[1].runOption = &Menu::enterStickConfigMenu;
        mainMenu[2].runOption = &Menu::enterTriggerConfigMenu;
        mainMenu[4].runOption = &Menu::enterSettingsMenu;
        mainMenu[5].runOption = &Menu::exitToPad;

        mCurrentMenu = mainMenu;
        mCurrentNumEntries = sizeof(mainMenu) / sizeof(MenuItem);
        mOffset = mPrevOffset;
        return 1;
    }

    int Menu::enterSettingsMenu(MenuItem *self)
    {
        settingsMenu[0].runOption = &Menu::enterMainMenu;
        settingsMenu[6].runOption = &Menu::toggleOption;

        mCurrentMenu = settingsMenu;
        mCurrentNumEntries = sizeof(settingsMenu) / sizeof(MenuItem);
        mPrevOffset = mOffset;
        mOffset = 0;
        return 1;
    }

    int Menu::enterStickConfigMenu(MenuItem *self)
    {
        stickConfigMenu[0].runOption = &Menu::enterMainMenu;
        stickConfigMenu[1].runOption = &Menu::enterStickCalibration;
        stickConfigMenu[2].runOption = &Menu::enterStickDeadzoneConfig;
        stickConfigMenu[3].runOption = &Menu::toggleOption;
        stickConfigMenu[4].runOption = &Menu::toggleOption;
        stickConfigMenu[5].runOption = &Menu::toggleOption;

        mCurrentMenu = stickConfigMenu;
        mCurrentNumEntries = sizeof(stickConfigMenu) / sizeof(MenuItem);
        mPrevOffset = mOffset;
        mOffset = 0;
        return 1;
    }

    int Menu::enterTriggerConfigMenu(MenuItem *self)
    {
        triggerConfigMenu[0].runOption = &Menu::enterMainMenu;
        //stickConfigMenu[1].runOption = &Menu::enterStickCalibration;
        triggerConfigMenu[3].runOption = &Menu::toggleOption;
        triggerConfigMenu[4].runOption = &Menu::toggleOption;
        triggerConfigMenu[5].runOption = &Menu::toggleOption;

        mCurrentMenu = triggerConfigMenu;
        mCurrentNumEntries = sizeof(triggerConfigMenu) / sizeof(MenuItem);
        mPrevOffset = mOffset;
        mOffset = 0;
        return 1;
    }

    int Menu::enterStickCalibration(MenuItem *self)
    {
        // draw stick calibration

        uint16_t color = 0xFFFF;

        while (!gpio_get(ButtonInfos[0].pin));

        mDisplay->clear();
        mDisplay->putString("Center stick,", 0, 0, color);
        mDisplay->putString("then press A!", 0, 1, color);
        mDisplay->update();

        sleep_ms(50);
        while (gpio_get(ButtonInfos[0].pin));

        adc_select_input(0); // X
        //uint8_t xCenter = adc_read() >> 4;

        adc_select_input(1); // Y
        //uint8_t yCenter = adc_read() >> 4;

        mDisplay->clear();
        mDisplay->putString("Move stick", 0, 0, color);
        mDisplay->putString("  around", 0, 1, color);
        mDisplay->putString("  a lot!", 0, 2, color);
        mDisplay->update();

        uint8_t xMin = 0x80;
        uint8_t xMax = 0x80;
        uint8_t yMin = 0x80;
        uint8_t yMax = 0x80;

        uint32_t start = to_ms_since_boot(get_absolute_time());
        while ((to_ms_since_boot(get_absolute_time()) - start) < 4000 ? true : gpio_get(ButtonInfos[0].pin)) {
            static bool prompt = true;
            static uint8_t xData = 0;
            static uint8_t yData = 0;

            adc_select_input(0); // X
            xData = adc_read() >> 4;

            adc_select_input(1); // Y
            yData = adc_read() >> 4;

            if (xData < xMin)
                xMin = xData;
            else if (xData > xMax)
                xMax = xData;

            if (yData < yMin)
                yMin = yData;
            else if (yData > yMax)
                yMax = yData;

            // printf("\033[H\033[2J");
            // printf("\033[36m");
            // printf("xRaw: 0x%04x  yRaw: 0x%02x\n", xRaw, yRaw);
            // printf("xData: 0x%02x  yData: 0x%02x\n", xData, yData);
            // printf("xMin: 0x%02x  xMax: 0x%02x yMin: 0x%02x yMax: 0x%02x\n", xMin, xMax, yMin, yMax);
            // sleep_ms(50);

            if ((to_ms_since_boot(get_absolute_time()) - start) >= 4000 && prompt) {
                prompt = false;
                mDisplay->putString("  Press A", 0, 3, color);
                mDisplay->putString(" when done!", 0, 4, color);
                mDisplay->update();
            }
        }

        // Write config values to flash
        //updateFlashData();

        return 1;
    }

    int Menu::enterStickDeadzoneConfig(MenuItem *self)
    {
        mRedraw = false;

        uint16_t color = 0xFFFF;
        char sdata[5] = {0};

        while(!gpio_get(ButtonInfos[0].pin));

        uint8_t xDeadzone = 0;
        uint8_t xAntiDeadzone = 0;
        uint8_t yDeadzone = 0;
        uint8_t yAntiDeadzone = 0;

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("X Deadzone", 0, 0, color);
            sprintf(sdata, "0x%02x", xDeadzone);
            mDisplay->putString(sdata, 3, 2, color);
            mDisplay->update();

            xDeadzone = setDeadzone(xDeadzone);

            sleep_ms(60);
        }

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("X", 5, 0, color);
            mDisplay->putString("AntiDeadzone", 0, 1, color);
            sprintf(sdata, "0x%02x", xAntiDeadzone);
            mDisplay->putString(sdata, 3, 3, color);
            mDisplay->update();

            xAntiDeadzone = setDeadzone(xAntiDeadzone);

            sleep_ms(60);
        }

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("Y Deadzone", 0, 0, color);
            sprintf(sdata, "0x%02x", yDeadzone);
            mDisplay->putString(sdata, 3, 2, color);
            mDisplay->update();

            yDeadzone = setDeadzone(yDeadzone);

            sleep_ms(60);
        }

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("Y", 5, 0, color);
            mDisplay->putString("AntiDeadzone", 0, 1, color);
            sprintf(sdata, "0x%02x", yAntiDeadzone);
            mDisplay->putString(sdata, 3, 3, color);
            mDisplay->update();

            yAntiDeadzone = setDeadzone(yAntiDeadzone);

            sleep_ms(60);
        }

        //updateFlashData();

        mDisplay->clear();

        mRedraw = true;

        return 1;
    }

    int Menu::setDeadzone(int deadzone)
    {
        int newDeadzone = 0;

        if (!gpio_get(ButtonInfos[4].pin))
        {
            if (deadzone < 128)
                newDeadzone = deadzone + 1;
        }
        else if (!gpio_get(ButtonInfos[5].pin))
        {
            if (deadzone > 0)
                newDeadzone = deadzone - 1;
        }
        else if (!gpio_get(ButtonInfos[6].pin))
        {
            if (deadzone > 7)
                newDeadzone = deadzone - 8;
        }
        else if (!gpio_get(ButtonInfos[7].pin))
        {
            if (deadzone < 121)
                newDeadzone = deadzone + 8;
        }

        return newDeadzone;
    }

    int Menu::toggleOption(MenuItem *self)
    {
        if (!strcmp(self->name, "OLED Flip     "))
        {
            if ((to_ms_since_boot(get_absolute_time()) - mFlipLockout) > 500)
            {

                if (self->type == entry_type::TOGGLE)
                {
                    self->isToggledOn = !(self->isToggledOn);
                }

                mFlipLockout = to_ms_since_boot(get_absolute_time());

                /*updateFlags();
                updateFlashData();
                if(oledType)
                    ssd1331_init();
                else ssd1306_init();*/
                sleep_ms(100);
            }
        } else {
            if (self->type == entry_type::TOGGLE)
            {
                self->isToggledOn = !(self->isToggledOn);
            }
        }

        return 1;
    }

    void Menu::updateMenu(int offset)
    {
        mDisplay->clear();

        for (uint8_t n = 0; n < mCurrentNumEntries; n++) {
            if(mCurrentMenu[n].isVisible)
            {
                mDisplay->putString(mCurrentMenu[n].name, 0, n + offset, 0xFFFF);
                if(mCurrentMenu[n].type == entry_type::TOGGLE)
                {
                    mDisplay->drawToggle(n + offset, 0xFFFF, mCurrentMenu[n].isToggledOn);
                }
            }
        }
        mDisplay->drawCursor(getSelectedEntry() + offset, 0xFFFF);
        mDisplay->update();
    }

    uint8_t Menu::getSelectedEntry()
    {
        uint8_t index = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isSelected)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getFirstVisibleEntry()
    {
        uint8_t index = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isVisible)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getLastVisibleEntry()
    {
        uint8_t index = 0;
        for (int i = mCurrentNumEntries - 1; i >= 0; i--)
        {
            if (mCurrentMenu[i].isVisible)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getFirstSelectableEntry()
    {
        uint8_t index = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getLastSelectableEntry()
    {
        uint8_t index = 0;
        for (int i = mCurrentNumEntries - 1; i >= 0; i--)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getNextSelectable(int selectedEntry)
    {
        uint8_t index = 0;
        for (int i = selectedEntry + 1; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }

        return index;
    }

    uint8_t Menu::getPrevSelectable(int selectedEntry)
    {
        uint8_t index = 0;
        for (int i = selectedEntry - 1; i >= 0; i--)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }

        return index;
    }

    void Menu::run()
    {
        int selectedEntry, firstVisibleEntry, lastVisibleEntry = 0;

        while(1)
        {
            selectedEntry = getSelectedEntry();

            // Wait for A button release (submenu rate-limit)
            while (!gpio_get(CTRL_PIN_A));

            sleep_ms(75); // Wait out switch bounce + rate-limiting

            if (!gpio_get(CTRL_PIN_DU)) //up
            {
                /* check currently selected entry
                if element is not the top one, deselect current entry
                and select the first enabled entry above it */
                if (selectedEntry) { // i.e. not 0
                    mCurrentMenu[selectedEntry].isSelected = false;
                    mCurrentMenu[selectedEntry - 1].isSelected = true;

                    firstVisibleEntry = getFirstVisibleEntry();
                    if ((selectedEntry == firstVisibleEntry) && (firstVisibleEntry)) {
                        mCurrentMenu[firstVisibleEntry + 4].isVisible = false;
                        mCurrentMenu[firstVisibleEntry - 1].isVisible = true;
                        mOffset++;
                    }
                }

            }
            else if (!gpio_get(CTRL_PIN_DD)) //down
            {
                /* check currently selected entry
                if entry is not the bottom one, deselect current entry
                and select first enabled entry below it */
                if (selectedEntry < mCurrentNumEntries - 1)
                {
                    mCurrentMenu[selectedEntry].isSelected = false;
                    mCurrentMenu[selectedEntry + 1].isSelected = true;

                    lastVisibleEntry = getLastVisibleEntry();
                    if ((selectedEntry == lastVisibleEntry) && (lastVisibleEntry < mCurrentNumEntries))
                    {
                        mCurrentMenu[lastVisibleEntry - 4].isVisible = false;
                        mCurrentMenu[lastVisibleEntry + 1].isVisible = true;
                        mOffset--;
                    }
                }
            }
            else if (!gpio_get(CTRL_PIN_A)) // A
            {
                /* check currently selected entry
                if entry is enabled, run entry's function
                entry functions should set currentMenu if they enter a submenu. */
                if (mCurrentMenu[selectedEntry].isEnabled)
                {
                    if (!(this->*mCurrentMenu[selectedEntry].runOption)(&mCurrentMenu[selectedEntry]))
                    {
                        break;
                    }
                }
            }

            if(mRedraw)
            {
                updateMenu(mOffset);
            }
        }
    }
}