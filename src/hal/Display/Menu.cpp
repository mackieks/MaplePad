#include "Menu.hpp"

namespace display
{
    Menu::Menu() :
        mCurrentNumEntries(sizeof(mainMenu) / sizeof(MenuItem)),
        mCurrentMenu(mainMenu)
    {

    }

    void Menu::updateMenu(std::shared_ptr<Display>  lcd)
    {
        lcd->clear();

        for (uint8_t n = 0; n < mCurrentNumEntries; n++) {
            lcd->putString(mCurrentMenu[n].name, 0, n + 0, 0xFFFF);
        }
        lcd->update();
    }

    uint8_t Menu::getSelectedEntry()
    {
        uint8_t selectedEntry = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].selected)
            {
                selectedEntry = i;
                break;
            }
        }
        return selectedEntry;
    }

    void Menu::showMenu(std::shared_ptr<Display> lcd)
    {
        while(1)
        {
            sleep_ms(50);
            updateMenu(lcd);
        }
    }
}