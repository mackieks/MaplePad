#pragma once

#include "Display.hpp"
#include <memory>

namespace display
{
    struct MenuItem
    {
        char name[15];
        bool selected;
    };

    static MenuItem mainMenu[6] = {
        {"Button Test   ", true},
        {"Stick Config  ", false}
        //{"Trigger Config"},
        //{"Edit VMU Color"}, // ssd1331 present
        //{"Settings      "},
        //{"Exit          "}
    };

    class Menu
    {
        public:
            Menu();

            void showMenu(std::shared_ptr<Display> lcd);

            void updateMenu(std::shared_ptr<Display> lcd);

        private:
            uint8_t getSelectedEntry();

        private:
            uint8_t mCurrentNumEntries;

            MenuItem *mCurrentMenu;
        
    };
}