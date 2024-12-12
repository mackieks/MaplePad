#include "Menu.hpp"

namespace display
{
    Menu::Menu(std::shared_ptr<Display> lcd) :
        mCurrentNumEntries(sizeof(mainMenu) / sizeof(MenuItem)),
        mCurrentMenu(mainMenu),
        mDisplay(lcd)
    {

    }

    void Menu::updateMenu(int offset)
    {
        mDisplay->clear();

        for (uint8_t n = 0; n < mCurrentNumEntries; n++) {
            if(mCurrentMenu[n].isVisible)
            {
                mDisplay->putString(mCurrentMenu[n].name, 0, n + offset, 0xFFFF);
            }
        }
        mDisplay->drawCursor(getSelectedEntry() + offset, 0xFFFF);
        mDisplay->update();
    }

    uint8_t Menu::getSelectedEntry()
    {
        uint8_t index = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isSelected)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getFirstVisibleEntry()
    {
        uint8_t index = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isVisible)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getLastVisibleEntry()
    {
        uint8_t index = 0;
        for (int i = mCurrentNumEntries - 1; i >= 0; i--)
        {
            if (mCurrentMenu[i].isVisible)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getFirstSelectableEntry()
    {
        uint8_t index = 0;
        for (int i = 0; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getLastSelectableEntry()
    {
        uint8_t index = 0;
        for (int i = mCurrentNumEntries - 1; i >= 0; i--)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }
        return index;
    }

    uint8_t Menu::getNextSelectable(int selectedEntry)
    {
        uint8_t index = 0;
        for (int i = selectedEntry + 1; i < mCurrentNumEntries; i++)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }

        return index;
    }

    uint8_t Menu::getPrevSelectable(int selectedEntry)
    {
        uint8_t index = 0;
        for (int i = selectedEntry - 1; i >= 0; i--)
        {
            if (mCurrentMenu[i].isSelectable)
            {
                index = i;
                break;
            }
        }

        return index;
    }

    void Menu::run()
    {
        int selectedEntry, firstVisibleEntry, lastVisibleEntry, offset = 0;

        while(true)
        {
            sleep_ms(100);
            
            selectedEntry = getSelectedEntry();

            if (!gpio_get(CTRL_PIN_DU)) //up
            {
                /* check currently selected entry
                if element is not the top one, deselect current entry
                and select the first enabled entry above it */
                if (selectedEntry) { // i.e. not 0
                    mCurrentMenu[selectedEntry].isSelected = false;
                    mCurrentMenu[selectedEntry - 1].isSelected = true;

                    firstVisibleEntry = getFirstVisibleEntry();
                    if ((selectedEntry == firstVisibleEntry) && (firstVisibleEntry)) {
                        mCurrentMenu[firstVisibleEntry + 4].isVisible = false;
                        mCurrentMenu[firstVisibleEntry - 1].isVisible = true;
                        offset++;
                    }
                }

            }
            else if (!gpio_get(CTRL_PIN_DD)) //down
            {
                /* check currently selected entry
                if entry is not the bottom one, deselect current entry
                and select first enabled entry below it */
                if (selectedEntry < mCurrentNumEntries - 1)
                {
                    mCurrentMenu[selectedEntry].isSelected = false;
                    mCurrentMenu[selectedEntry + 1].isSelected = true;

                    lastVisibleEntry = getLastVisibleEntry();
                    if ((selectedEntry == lastVisibleEntry) && (lastVisibleEntry < mCurrentNumEntries))
                    {
                        mCurrentMenu[lastVisibleEntry - 4].isVisible = false;
                        mCurrentMenu[lastVisibleEntry + 1].isVisible = true;
                        offset--;
                    }
                }
            }

            updateMenu(offset);
        }
    }
}