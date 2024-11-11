#pragma once

#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

#include <string.h>

namespace client
{
class DreamcastKeyboard : public DreamcastPeripheralFunction
{
public:
    enum class Language : uint8_t
    {
        Japan = 1,
        America = 2,
        England = 3,
        Germany = 4,
        France = 5,
        Italy = 6,
        Spain = 7,
        Sweden = 8,
        Switzerland = 9,
        Holland = 10,
        Portugal = 11,
        LatinAmerica = 12,
        CanadianFrench = 13,
        Russia = 14,
        China = 15,
        Korea = 16
    };

    enum class Type : uint8_t
    {
        Key89 = 1,
        Key92 = 2,
        Key101 = 3,
        Key102 = 4,
        Key104 = 5,
        Key105 = 6,
        Key106 = 7,
        Key109 = 8,
        Key87 = 9,
        Key88 = 10
    };

    struct ChangeKeyBits
    {
        bool leftCtrl;
        bool leftShift;
        bool leftAlt;
        bool leftGui;
        bool rightCtrl;
        bool rightShift;
        bool rightAlt;
        bool s2;
    };

    struct LedState
    {
        bool numLockLedOn;
        bool capsLockLedOn;
        bool scrollLockLedOn;
        bool kanaLedOn;
        bool powerLedOn;
        bool shiftLedOn;
    };

    inline DreamcastKeyboard(Language language,
                             Type type,
                             bool hasNumLockLed,
                             bool hasCapsLockLed,
                             bool hasScrollLockLed,
                             bool hasKanaLed,
                             bool hasPowerLed,
                             bool hasShiftLed,
                             bool keyboardControlsLed) :
        DreamcastPeripheralFunction(DEVICE_FN_KEYBOARD),
        mLanguage(language),
        mType(type),
        mHasNumLockLed(hasNumLockLed),
        mHasCapsLockLed(hasCapsLockLed),
        mHasScrollLockLed(hasScrollLockLed),
        mHasKanaLed(hasKanaLed),
        mHasPowerLed(hasPowerLed),
        mHasShiftLed(hasShiftLed),
        mKeyboardControlsLed(keyboardControlsLed),
        mPressedChangeKeys(0),
        mLedState(),
        mPressedKeys()
    {}

    inline virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final
    {
        const uint8_t cmd = in.frame.command;
        switch (cmd)
        {
            case COMMAND_GET_CONDITION:
            {
                out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                out.reservePayload(3);
                out.appendPayload(getFunctionCode());
                uint32_t payload[2] = {
                    (mPressedChangeKeys << 24)
                        | ((mLedState.shiftLedOn ? 0x80 : 0) << 16)
                        | ((mLedState.powerLedOn ? 0x40 : 0) << 16)
                        | ((mLedState.kanaLedOn ? 0x20 : 0) << 16)
                        | ((mLedState.scrollLockLedOn ? 0x04 : 0) << 16)
                        | ((mLedState.capsLockLedOn ? 0x02 : 0) << 16)
                        | ((mLedState.numLockLedOn ? 0x01 : 0) << 16)
                        | (mPressedKeys[0] << 8)
                        | mPressedKeys[1],
                    (mPressedKeys[2] << 24)
                        | (mPressedKeys[3] << 16)
                        | (mPressedKeys[4] << 8)
                        | mPressedKeys[5]
                };
                out.appendPayload(payload, 2);
                return true;
            }
            break;

            case COMMAND_SET_CONDITION:
            {
                if (in.payload.size() > 1)
                {
                    uint8_t ledSetting = in.payload[1] >> 24;
                    mLedState.shiftLedOn = ((ledSetting & 0x80) != 0);
                    mLedState.powerLedOn = ((ledSetting & 0x40) != 0);
                    mLedState.kanaLedOn = ((ledSetting & 0x20) != 0);
                    mLedState.scrollLockLedOn = ((ledSetting & 0x04) != 0);
                    mLedState.capsLockLedOn = ((ledSetting & 0x02) != 0);
                    mLedState.numLockLedOn = ((ledSetting & 0x01) != 0);

                    out.frame.command = COMMAND_RESPONSE_ACK;
                    return true;
                }
            }
            break;
        }
        return false;
    }

    inline virtual void reset() final
    {
        mLedState.numLockLedOn = false;
        mLedState.capsLockLedOn = false;
        mLedState.scrollLockLedOn = false;
        mLedState.kanaLedOn = false;
        mLedState.powerLedOn = false;
        mLedState.shiftLedOn = false;
    }

    inline virtual uint32_t getFunctionDefinition() final
    {
        // My 104 key keyboard: 02050080
        return ((static_cast<uint32_t>(mLanguage) & 0xFF) << 24)
            | ((static_cast<uint32_t>(mType) & 0xFF) << 16)
            | ((mHasNumLockLed ? 0x01 : 0) << 8)
            | ((mHasCapsLockLed ? 0x02 : 0) << 8)
            | ((mHasScrollLockLed ? 0x04 : 0) << 8)
            | ((mHasKanaLed ? 0x20 : 0) << 8)
            | ((mHasPowerLed ? 0x40 : 0) << 8)
            | ((mHasShiftLed ? 0x80 : 0) << 8)
            | (mKeyboardControlsLed ? 0x80 : 0);
    }

    inline void setKeys(ChangeKeyBits changeKeys, uint8_t keys[6])
    {
        mPressedChangeKeys = (changeKeys.s2 ? 0x80 : 0)
                        | (changeKeys.rightAlt ? 0x40 : 0)
                        | (changeKeys.rightShift ? 0x20 : 0)
                        | (changeKeys.rightCtrl ? 0x10 : 0)
                        | (changeKeys.leftGui ? 0x08 : 0)
                        | (changeKeys.leftAlt ? 0x04 : 0)
                        | (changeKeys.leftShift ? 0x02 : 0)
                        | (changeKeys.leftCtrl ? 0x01 : 0);
        memcpy(mPressedKeys, keys, sizeof(mPressedKeys));
    }

    inline void setKeys(uint8_t changeKeys, uint8_t keys[6])
    {
        mPressedChangeKeys = changeKeys;
        memcpy(mPressedKeys, keys, sizeof(mPressedKeys));
    }

    inline const LedState& getLedState()
    {
        return mLedState;
    }

private:
    const Language mLanguage;
    const Type mType;
    const bool mHasNumLockLed;
    const bool mHasCapsLockLed;
    const bool mHasScrollLockLed;
    const bool mHasKanaLed;
    const bool mHasPowerLed;
    const bool mHasShiftLed;
    const bool mKeyboardControlsLed;

    uint8_t mPressedChangeKeys;
    LedState mLedState;
    //! Currently pressed set of keys - follows the same format as a HID keyboard
    uint8_t mPressedKeys[6];
};
}