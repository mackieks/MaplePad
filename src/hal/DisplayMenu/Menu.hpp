#pragma once

#include "Display.hpp"
#include "pico_configurations.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hal/System/SystemMemory.hpp"
#include "NonVolatilePicoSystemMemory.hpp"

#include <memory>

namespace display
{
    static const uint32_t MEMORY_SIZE_BYTES = 128 * 1024;

    enum entry_type { SUBMENU, TOGGLE, FUNCTION, ADJUSTABLE, INERT };

    // Forward declare Menu class
    class Menu;

    class MenuItem {

        public:
            const char* name;
            enum entry_type type;
            bool isSelected;
            bool isSelectable;
            bool isVisible;
            bool isEnabled;
            bool isToggledOn;
            // Pointer to Menu's non-static member function
            int (Menu::*runOption)(MenuItem* self);  // Correct definition of the member function pointer

            //! Constructor - Item Name, Selected, Selectable, Visible, Enabled
            MenuItem(const char* itemName = "", entry_type entryType = entry_type::INERT, bool selected = false, bool selectable = false, 
                bool visible = true, bool enabled = true, bool toggledOn = false)
                : name(itemName), type(entryType), isSelected(selected), isSelectable(selectable), 
                isVisible(visible), isEnabled(enabled), isToggledOn(toggledOn) {}
    };

    class Menu
    {
        public:
            Menu(std::shared_ptr<Display> lcd);

            void run();

            void readFlash();

        private:
            void updateMenu(int offset);

            uint8_t getSelectedEntry();

            uint8_t getFirstVisibleEntry();

            uint8_t getLastVisibleEntry();

            uint8_t getFirstSelectableEntry();

            uint8_t getLastSelectableEntry();

            uint8_t getNextSelectable(int selectedEntry);

            uint8_t getPrevSelectable(int selectedEntry);

            int exitToPad(MenuItem *self);

            int saveAndExitToPad(MenuItem *self);

            int enterMainMenu(MenuItem *self);

            int enterSettingsMenu(MenuItem *self);

            int enterStickConfigMenu(MenuItem *self);

            int enterStickCalibration(MenuItem *self);

            int enterStickDeadzoneConfig(MenuItem *self);

            int enterTriggerConfigMenu(MenuItem *self);

            int enterTriggerCalibration(MenuItem *self);

            int toggleOption(MenuItem *self);

            int setDeadzone(int deadzone);

            void updateToggles();

            void loadToggles();

            void updateFlashData();

        private:
            uint8_t mCurrentNumEntries;

            MenuItem *mCurrentMenu;

            std::shared_ptr<Display> mDisplay;

            uint8_t mOffset = 0;

            uint8_t mPrevOffset = 0;

            bool mRedraw = true;

            uint32_t mFlipLockout = 0;

            std::shared_ptr<NonVolatilePicoSystemMemory> mSystemMemory;

            uint32_t mFlashOffset = PICO_FLASH_SIZE_BYTES - (MEMORY_SIZE_BYTES * 9);

            uint8_t mFlashData[64];

            uint8_t mXCenter = 0;
            uint8_t mXMin = 0;
            uint8_t mXMax = 0;
            uint8_t mYCenter = 0;
            uint8_t mYMin = 0;
            uint8_t mYMax = 0;
            uint8_t mLMin = 0;
            uint8_t mLMax = 0;
            uint8_t mRMin = 0;
            uint8_t mRMax = 0;
            uint8_t mInvertX = 0;
            uint8_t mInvertY = 0;
            uint8_t mInvertL = 0;
            uint8_t mInvertR = 0;
            uint8_t mFirstBoot = 0;
            uint8_t mCurrentPage = 0;
            uint8_t mRumbleEnable = 0;
            uint8_t mVmuEnable = 0;
            uint8_t mOledFlip = 0;
            uint8_t mSwapXY = 0;
            uint8_t mSwapLR = 0;
            uint8_t mOledType = 0;
            uint8_t mTriggerMode = 0;
            uint8_t mXDeadzone = 0;
            uint8_t mXAntiDeadzone = 0;
            uint8_t mYDeadzone = 0;
            uint8_t mYAntiDeadzone = 0;
            uint8_t mLDeadzone = 0;
            uint8_t mLAntiDeadzone = 0;
            uint8_t mRDeadzone = 0;
            uint8_t mRAntiDeadzone = 0;
            uint8_t mAutoResetEnable = 0;
            uint8_t mAutoResetTimer = 0;
            uint8_t mVersion = 0;

            //OLED can only fit 5 rows of text at a time
            MenuItem mainMenu[7] = {
                MenuItem("Button Test   ", entry_type::SUBMENU, false, true, true),
                MenuItem("Stick Config  ", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Trigger Config", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Edit VMU Color", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Settings      ", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Exit          ", entry_type::INERT, false, false, false, true),
                MenuItem("Save & Exit   ", entry_type::INERT, false, false, false, true)
            };

            MenuItem settingsMenu[10] = {
                MenuItem("Back          ", entry_type::FUNCTION, true, false, true, true),
                MenuItem("Boot Video    ", entry_type::INERT, false, false, true, true),
                MenuItem("Rumble        ", entry_type::INERT, false, false, true, true),
                MenuItem("VMU           ", entry_type::INERT, false, false, true, true),
                MenuItem("UI Color      ", entry_type::INERT, false, false, true, true),
                MenuItem("OLED:         ", entry_type::INERT, false, false, false, true),
                MenuItem("OLED Flip     ", entry_type::TOGGLE, false, false, false, true),
                MenuItem("Autoreset     ", entry_type::INERT, false, false, false, true),
                MenuItem("Adjust Timeout", entry_type::INERT, false, false, false, true),
                MenuItem("FW:        2.0", entry_type::INERT, false, false, false, true)
            };

            MenuItem stickConfigMenu[6] = {
                MenuItem("Back          ", entry_type::FUNCTION, true, false, true, true),
                MenuItem("Calibration   ", entry_type::FUNCTION, false, false, true, true),
                MenuItem("Deadzone Edit ", entry_type::FUNCTION, false, false, true, true),
                MenuItem("Invert X      ", entry_type::TOGGLE, false, false, true, true),
                MenuItem("Invert Y      ", entry_type::TOGGLE, false, false, true, true),
                MenuItem("Swap X&Y      ", entry_type::TOGGLE, false, false, false, true)
            };

            MenuItem triggerConfigMenu[6] = {
                MenuItem("Back          ", entry_type::FUNCTION, true, false, true, true),
                MenuItem("Calibration   ", entry_type::FUNCTION, false, false, true, true),
                MenuItem("Deadzone Edit ", entry_type::FUNCTION, false, false, true, true),
                MenuItem("Invert X      ", entry_type::TOGGLE, false, false, true, true),
                MenuItem("Invert Y      ", entry_type::TOGGLE, false, false, true, true),
                MenuItem("Swap X&Y      ", entry_type::TOGGLE, false, false, false, true)
            };
        
    };
}