#include "Menu.hpp"

namespace display
{
    Menu::Menu()
    {

    }

    void Menu::updateMenu(std::shared_ptr<Display>  lcd)
    {
        lcd->clear();

        uint8_t numEntries = sizeof(display::mainMenu) / sizeof(MenuOptions);

        for (uint8_t n = 0; n < numEntries; n++) {
            lcd->putString(display::mainMenu[n].name, 0, n + 0, 0xFFFF);
        }
        lcd->update();
    }

    void Menu::showMenu(std::shared_ptr<Display> lcd)
    {
        while(1)
        {
            sleep_ms(100);
            updateMenu(lcd);
        }
    }
}