#pragma once

#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"

#include <string.h>

namespace client
{
class DreamcastScreen : public DreamcastPeripheralFunction
{
public:
    typedef void (*ScreenFn)(const uint32_t* screen, uint32_t len);

public:
    DreamcastScreen(ScreenFn callback, uint8_t width = 48, uint8_t height = 32);

    //! Handle packet meant for this peripheral function
    //! @param[in] in  The packet read from the Maple Bus
    //! @param[out] out  The packet to write to the Maple Bus when true is returned
    //! @returns true iff the packet was handled
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    //! Called when player index changed or timeout occurred
    virtual void reset() final;

    //! @returns the function definition for this peripheral function
    virtual uint32_t getFunctionDefinition() final;

private:
    //! This screen's pixel width
    const uint8_t mWidth;
    //! This screen's pixel height
    const uint8_t mHeight;
    //! Callback function which sets screen data
    ScreenFn mCallback;

};
}
