#pragma once

#include "Display.hpp"
#include "pico_configurations.h"
#include <memory>

namespace display
{
    struct MenuItem
    {
        char name[15];
        bool isSelected;
        bool isSelectable;
        bool isVisible;
    };

    static MenuItem mainMenu[6] = {
        //OLED can only fit 5 rows of text at a time
        //Name, isSelected, isSelectable, isVisible
        {"Button Test   ", true, false, true},
        {"Stick Config  ", false, false, true},
        {"Trigger Config", false, false, true},
        {"Edit VMU Color", false, false, true},
        {"Settings      ", false, false, true},
        {"Exit          ", false, false, false}
    };

    class Menu
    {
        public:
            Menu(std::shared_ptr<Display> lcd);

            void runMenu();

            void updateMenu(int offset);

        private:
            uint8_t getSelectedEntry();

            uint8_t getFirstVisibleEntry();

            uint8_t getLastVisibleEntry();

            uint8_t getFirstSelectableEntry();

            uint8_t getLastSelectableEntry();

            uint8_t getNextSelectable(int selectedEntry);

            uint8_t getPrevSelectable(int selectedEntry);

        private:
            uint8_t mCurrentNumEntries;

            MenuItem *mCurrentMenu;

            std::shared_ptr<Display> mDisplay;
        
    };
}