#include "Font.hpp"

namespace display
{

    Font::Font()
    {
    }

    tImage* Font::getFontImage(int key)
    {
        auto it = mFontMap.find(key);
        if (it != mFontMap.end())
        {
            return &it->second;
        }
        return nullptr;
    }
}