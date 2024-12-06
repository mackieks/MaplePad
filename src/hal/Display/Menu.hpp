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
        {"Stick Config  "}
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
        
    };
}