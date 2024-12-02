#include "Menu.hpp"

namespace display
{
    Menu::Menu()
    {

    }

    void Menu::updateMenu(std::unique_ptr<Display>& lcd)
    {
        lcd->clear();

        uint8_t numEntries = sizeof(display::mainMenu) / sizeof(MenuOptions);

        for (uint8_t n = 0; n < numEntries; n++) {
            lcd->putString(display::mainMenu[n].name, 0, n + 0, 0x0000);
        }
        lcd->update();
    }

    void Menu::showMenu(std::unique_ptr<Display>& lcd)
    {
        while(1)
        {
            updateMenu(lcd);
            sleep_ms(500);
        }
    }
}