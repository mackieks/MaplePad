#pragma once

#include "Display.hpp"
#include "pico_configurations.h"
#include <memory>
#include "hardware/adc.h"

namespace display
{
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

            int enterMainMenu(MenuItem *self);

            int enterSettingsMenu(MenuItem *self);

            int enterStickConfigMenu(MenuItem *self);

            int enterStickCalibration(MenuItem *self);

            int enterStickDeadzoneConfig(MenuItem *self);

            int enterTriggerConfigMenu(MenuItem *self);

            int toggleOption(MenuItem *self);

            int setDeadzone(int deadzone);

        private:
            uint8_t mCurrentNumEntries;

            MenuItem *mCurrentMenu;

            std::shared_ptr<Display> mDisplay;

            uint8_t mOffset = 0;

            uint8_t mPrevOffset = 0;

            bool mRedraw = true;

            uint32_t mFlipLockout = 0;

            //OLED can only fit 5 rows of text at a time
            MenuItem mainMenu[6] = {
                MenuItem("Button Test   ", entry_type::SUBMENU, false, true, true),
                MenuItem("Stick Config  ", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Trigger Config", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Edit VMU Color", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Settings      ", entry_type::SUBMENU, false, false, true, true),
                MenuItem("Exit          ", entry_type::INERT, false, false, false, true)
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