#pragma once

#include "Display.hpp"
#include "pico_configurations.h"
#include <memory>

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
            // Pointer to Menu's non-static member function
            int (Menu::*runOption)(MenuItem* self);  // Correct definition of the member function pointer

            //! Constructor - Item Name, Selected, Selectable, Visible, Enabled
            MenuItem(const char* itemName = "", bool selected = false, bool selectable = false, bool visible = true, bool enabled = true)
                : name(itemName), isSelected(selected), isSelectable(selectable), isVisible(visible), isEnabled(enabled) {}
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

        private:
            uint8_t mCurrentNumEntries;

            MenuItem *mCurrentMenu;

            std::shared_ptr<Display> mDisplay;

            uint8_t mOffset = 0;

            uint8_t mPrevOffset = 0;

            //OLED can only fit 5 rows of text at a time
            MenuItem mainMenu[6] = {
                MenuItem("Button Test   ", true, false, true, true),
                MenuItem("Stick Config  ", false, false, true, true),
                MenuItem("Trigger Config", false, false, true, true),
                MenuItem("Edit VMU Color", false, false, true, true),
                MenuItem("Settings      ", false, false, true, true),
                MenuItem("Exit          ", false, false, false, true)
            };

            MenuItem settingsMenu[10] = {
                MenuItem("Back          ", true, false, true, true),
                MenuItem("Boot Video    ", false, false, true, true),
                MenuItem("Rumble        ", false, false, true, true),
                MenuItem("VMU           ", false, false, true, true),
                MenuItem("UI Color      ", false, false, true, true),
                MenuItem("OLED:         ", false, false, false, true),
                MenuItem("OLED Flip     ", false, false, false, true),
                MenuItem("Autoreset     ", false, false, false, true),
                MenuItem("Adjust Timeout", false, false, false, true),
                MenuItem("FW:        2.0", false, false, false, true)
            };
        
    };
}