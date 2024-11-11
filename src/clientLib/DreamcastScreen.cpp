#include "DreamcastScreen.hpp"

namespace client
{

DreamcastScreen::DreamcastScreen(ScreenFn callback, uint8_t width, uint8_t height) :
    DreamcastPeripheralFunction(DEVICE_FN_LCD),
    mWidth(width),
    mHeight(height),
    mCallback(callback)
{}

//! Handle packet meant for this peripheral function
//! @param[in] in  The packet read from the Maple Bus
//! @param[out] out  The packet to write to the Maple Bus when true is returned
//! @returns true iff the packet was handled
bool DreamcastScreen::handlePacket(const MaplePacket& in, MaplePacket& out)
{
    const uint8_t cmd = in.frame.command;
    switch (cmd)
    {
        case COMMAND_GET_MEMORY_INFORMATION:
        {
            out.frame.command = COMMAND_RESPONSE_DATA_XFER;
            out.reservePayload(2);
            out.appendPayload(getFunctionCode());
            out.appendPayload(((mWidth - 1) << 24) | ((mHeight - 1) << 16) | 0x1002);
            return true;
        }
        break;

        case COMMAND_BLOCK_WRITE:
        {
            if (in.payload.size() > 2)
            {
                if (mCallback)
                {
                    mCallback(in.payload.data() + 2, in.payload.size() - 2);
                }
                out.frame.command = COMMAND_RESPONSE_ACK;
                return true;
            }
        }
        break;

        case COMMAND_SET_CONDITION:
        {
            out.frame.command = COMMAND_RESPONSE_DATA_XFER;
            out.reservePayload(2);
            out.appendPayload(getFunctionCode());
            out.appendPayload(0);
            return true;
        }
        break;

        default:
        {
        }
        break;
    }

    return false;
}

//! Called when player index changed or timeout occurred
void DreamcastScreen::reset()
{
    if (mCallback)
    {
        uint32_t words = ((mWidth * mHeight) + 31) / 32;
        uint32_t dat[words] = {};
        mCallback(dat, words);
    }
}

//! @returns the function definition for this peripheral function
uint32_t DreamcastScreen::getFunctionDefinition()
{
    uint32_t numBytes = (mWidth * mHeight) / 8;
    uint32_t numBlocks = numBytes / 32;

    return 0x00001000 | (((numBlocks - 1) & 0xFF) << 16);
}

}
