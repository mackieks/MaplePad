#pragma once

#include "Display.hpp"
#include <memory>

namespace display
{
    struct MenuOptions
    {
        char name[15];
    };

    static MenuOptions mainMenu[6] = {
        {"Button Test   "},
        {"Stick Config  "},
        {"Trigger Config"},
        {"Edit VMU Color"}, // ssd1331 present
        {"Settings      "},
        {"Exit          "}
    };

    class Menu
    {
        public:
            Menu();

            void showMenu(std::unique_ptr<Display>& lcd);
        
        private:
            void updateMenu(std::unique_ptr<Display>& lcd);
    };
}