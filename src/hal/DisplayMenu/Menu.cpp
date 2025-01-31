#include "Menu.hpp"

namespace display
{

    Menu::Menu(std::shared_ptr<Display> lcd, std::shared_ptr<NonVolatilePicoSystemMemory> mem) :
        mCurrentNumEntries(sizeof(mainMenu) / sizeof(MenuItem)),
        mCurrentMenu(mainMenu),
        mDisplay(lcd),
        mSystemMemory(mem)
    {
        enterMainMenu(NULL);
    }

    int Menu::exitToPad(MenuItem *self)
    {
        return 0;
    }

    int Menu::saveAndExitToPad(MenuItem *self)
    {
        //Set member vars for toggles before saving
        updateToggles();
        updateFlashData();
        return 0;
    }

    int Menu::enterMainMenu(MenuItem *self)
    {
        mainMenu[1].runOption = &Menu::enterStickConfigMenu;
        mainMenu[2].runOption = &Menu::enterTriggerConfigMenu;
        mainMenu[4].runOption = &Menu::enterSettingsMenu;
        mainMenu[5].runOption = &Menu::exitToPad;
        mainMenu[6].runOption = &Menu::saveAndExitToPad;

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
        triggerConfigMenu[1].runOption = &Menu::enterTriggerCalibration;
        triggerConfigMenu[3].runOption = &Menu::toggleOption;
        triggerConfigMenu[4].runOption = &Menu::toggleOption;
        triggerConfigMenu[5].runOption = &Menu::toggleOption;

        mCurrentMenu = triggerConfigMenu;
        mCurrentNumEntries = sizeof(triggerConfigMenu) / sizeof(MenuItem);
        mPrevOffset = mOffset;
        mOffset = 0;
        return 1;
    }

    void Menu::updateToggles()
    {
        mInvertX = stickConfigMenu[3].isToggledOn;
        mInvertY = stickConfigMenu[4].isToggledOn;
        mSwapXY = stickConfigMenu[5].isToggledOn;
        mInvertL = triggerConfigMenu[3].isToggledOn;
        mInvertR = triggerConfigMenu[4].isToggledOn;
        mSwapLR = triggerConfigMenu[5].isToggledOn;
        mRumbleEnable = settingsMenu[2].isToggledOn;
        mVmuEnable = settingsMenu[3].isToggledOn;
        mOledFlip = settingsMenu[6].isToggledOn;
        mAutoResetEnable = settingsMenu[7].isToggledOn;
    }

    void Menu::loadToggles()
    {
        stickConfigMenu[3].isToggledOn = mInvertX;
        stickConfigMenu[4].isToggledOn = mInvertY;
        stickConfigMenu[5].isToggledOn = mSwapXY;
        triggerConfigMenu[3].isToggledOn = mInvertL;
        triggerConfigMenu[4].isToggledOn = mInvertR;
        triggerConfigMenu[5].isToggledOn = mSwapLR;
        settingsMenu[2].isToggledOn = mRumbleEnable;
        settingsMenu[3].isToggledOn = mVmuEnable;
        settingsMenu[6].isToggledOn = mOledFlip;
        settingsMenu[7].isToggledOn = mAutoResetEnable;
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
        mXCenter = adc_read() >> 4;

        adc_select_input(1); // Y
        mYCenter = adc_read() >> 4;

        mDisplay->clear();
        mDisplay->putString("Move stick", 0, 0, color);
        mDisplay->putString("  around", 0, 1, color);
        mDisplay->putString("  a lot!", 0, 2, color);
        mDisplay->update();

        uint32_t start = to_ms_since_boot(get_absolute_time());
        while ((to_ms_since_boot(get_absolute_time()) - start) < 4000 ? true : gpio_get(ButtonInfos[0].pin)) {
            static bool prompt = true;
            static uint8_t xData = 0;
            static uint8_t yData = 0;

            adc_select_input(0); // X
            xData = adc_read() >> 4;

            adc_select_input(1); // Y
            yData = adc_read() >> 4;

            if (xData < mXMin)
                mXMin = xData;
            else if (xData > mXMax)
                mXMax = xData;

            if (yData < mYMin)
                mYMin = yData;
            else if (yData > mYMax)
                mYMax = yData;

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

    int Menu::enterTriggerCalibration(MenuItem *self)
    {
        // draw trigger calibration
        mRedraw = 0; // Disable redrawMenu

        uint16_t color = 0xFFFF;

        while (!gpio_get(ButtonInfos[0].pin));

        mDisplay->clear();
        mDisplay->putString("leave", 0, 0, color);
        mDisplay->putString("triggers idle", 0, 1, color);
        mDisplay->putString("and press A", 0, 2, color);
        mDisplay->update();

        sleep_ms(500);
        while (gpio_get(ButtonInfos[0].pin));

        adc_select_input(2); // L
        mLMin = adc_read() >> 4;

        adc_select_input(3); // R
        mRMin = adc_read() >> 4;

        mDisplay->clear();
        mDisplay->putString("hold lMax", 0, 0, color);
        mDisplay->putString("and press A", 0, 1, color);
        mDisplay->update();

        sleep_ms(500);
        while (gpio_get(ButtonInfos[0].pin));

        adc_select_input(2); // lMax
        mLMax = adc_read() >> 4;

        mDisplay->clear();
        mDisplay->putString("hold rMax", 0, 0, color);
        mDisplay->putString("and press A", 0, 1, color);
        mDisplay->update();

        sleep_ms(500);
        while (gpio_get(ButtonInfos[0].pin));

        adc_select_input(3); // rMax
        mRMax = adc_read() >> 4;

        if (mLMin > mLMax) {
            uint temp = mLMin;
            mLMin = mLMax;
            mLMax = temp;
        }

        if (mRMin > mRMax) {
            uint temp = mRMin;
            mRMin = mRMax;
            mRMax = temp;
        }

        // Write config values to flash
        //updateFlashData();

        mRedraw = 1;

        return 1;
    }

    int Menu::enterStickDeadzoneConfig(MenuItem *self)
    {
        uint16_t color = 0xFFFF;
        char sdata[5] = {0};

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("X Deadzone", 0, 0, color);
            sprintf(sdata, "0x%02x", mXDeadzone);
            mDisplay->putString(sdata, 3, 2, color);
            mDisplay->update();

            mXDeadzone = setDeadzone(mXDeadzone);

            sleep_ms(60);
        }

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("X", 5, 0, color);
            mDisplay->putString("AntiDeadzone", 0, 1, color);
            sprintf(sdata, "0x%02x", mXAntiDeadzone);
            mDisplay->putString(sdata, 3, 3, color);
            mDisplay->update();

            mXAntiDeadzone = setDeadzone(mXAntiDeadzone);

            sleep_ms(60);
        }

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("Y Deadzone", 0, 0, color);
            sprintf(sdata, "0x%02x", mYDeadzone);
            mDisplay->putString(sdata, 3, 2, color);
            mDisplay->update();

            mYDeadzone = setDeadzone(mYDeadzone);

            sleep_ms(60);
        }

        while(!gpio_get(ButtonInfos[0].pin));

        while (gpio_get(ButtonInfos[0].pin))
        {
            mDisplay->clear();
            mDisplay->putString("Y", 5, 0, color);
            mDisplay->putString("AntiDeadzone", 0, 1, color);
            sprintf(sdata, "0x%02x", mYAntiDeadzone);
            mDisplay->putString(sdata, 3, 3, color);
            mDisplay->update();

            mYAntiDeadzone = setDeadzone(mYAntiDeadzone);

            sleep_ms(60);
        }

        //updateFlashData();

        mDisplay->clear();

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

    void Menu::readFlash()
    {
        mFlashData = mSystemMemory->fetchSettingsFromFlash();

        mXCenter = mFlashData[0];
        mXMin = mFlashData[1];
        mXMax = mFlashData[2];
        mYCenter = mFlashData[3];
        mYMin = mFlashData[4];
        mYMax = mFlashData[5];
        mLMin = mFlashData[6];
        mLMax = mFlashData[7];
        mRMin = mFlashData[8];
        mRMax = mFlashData[9];
        mInvertX = mFlashData[10];
        mInvertY = mFlashData[11];
        mInvertL = mFlashData[12];
        mInvertR = mFlashData[13];
        mFirstBoot = mFlashData[14];
        mCurrentPage = mFlashData[15];
        mRumbleEnable = mFlashData[16];
        mVmuEnable = mFlashData[17];
        mOledFlip = mFlashData[18];
        mSwapXY = mFlashData[19];
        mSwapLR = mFlashData[20];
        mOledType = mFlashData[21];
        mTriggerMode = mFlashData[22];
        mXDeadzone = mFlashData[23];
        mXAntiDeadzone = mFlashData[24];
        mYDeadzone = mFlashData[25];
        mYAntiDeadzone = mFlashData[26];
        mLDeadzone = mFlashData[27];
        mLAntiDeadzone = mFlashData[28];
        mRDeadzone = mFlashData[29];
        mRAntiDeadzone = mFlashData[30];
        mAutoResetEnable = mFlashData[31];
        mAutoResetTimer = mFlashData[32];
        mVersion = mFlashData[33];
    }

    void Menu::updateFlashData()
    {   
        mFlashData[0] = mXCenter;
        mFlashData[1] = mXMin;
        mFlashData[2] = mXMax;
        mFlashData[3] = mYCenter;
        mFlashData[4] = mYMin;
        mFlashData[5] = mYMax;
        mFlashData[6] = mLMin;
        mFlashData[7] = mLMax;
        mFlashData[8] = mRMin;
        mFlashData[9] = mRMax;
        mFlashData[10] = mInvertX;
        mFlashData[11] = mInvertY;
        mFlashData[12] = mInvertL;
        mFlashData[13] = mInvertR;
        mFlashData[14] = mFirstBoot;
        mFlashData[15] = mCurrentPage;
        mFlashData[16] = mRumbleEnable;
        mFlashData[17] = mVmuEnable;
        mFlashData[18] = mOledFlip;
        mFlashData[19] = mSwapXY;
        mFlashData[20] = mSwapLR;
        mFlashData[21] = mOledType;
        mFlashData[22] = mTriggerMode;
        mFlashData[23] = mXDeadzone;
        mFlashData[24] = mXAntiDeadzone;
        mFlashData[25] = mYDeadzone;
        mFlashData[26] = mYAntiDeadzone;
        mFlashData[27] = mLDeadzone;
        mFlashData[28] = mLAntiDeadzone;
        mFlashData[29] = mRDeadzone;
        mFlashData[30] = mRAntiDeadzone;
        mFlashData[31] = mAutoResetEnable;
        mFlashData[32] = mAutoResetTimer;
        mFlashData[33] = 0x0C;

        mSystemMemory->writeSettingsToFlash(mFlashData);

        delete mFlashData; //free up space
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

        readFlash();

        loadToggles();

        while(1)
        {
            selectedEntry = getSelectedEntry();

            // Wait for A button release (submenu rate-limit)
            while (!gpio_get(CTRL_PIN_A));

            sleep_ms(50); // Wait out switch bounce + rate-limiting

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

            updateMenu(mOffset);
        }
    }
}