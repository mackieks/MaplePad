#pragma once

#include <memory>
#include <vector>
#include <bitset>
#include <assert.h>

#include "hal/MapleBus/MaplePacket.hpp"

namespace client
{

class DreamcastPeripheralFunction
{
public:
    //! Constructor
    //! @param[in] functionCode  The function code (mask) for this peripheral function
    inline DreamcastPeripheralFunction(uint32_t functionCode) :
        mFunctionCode(functionCode)
    {
        // One and only one bit may be set for the function code
        std::bitset<32> leftBits(mFunctionCode);
        assert(leftBits.count() == 1);
    }

    //! Virtual destructor
    virtual inline ~DreamcastPeripheralFunction() {}

    //! Handle packet meant for this peripheral function
    //! @param[in] in  The packet read from the Maple Bus
    //! @param[out] out  The packet to write to the Maple Bus when true is returned
    //! @returns true iff the packet was handled
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) = 0;

    //! Called when player index changed or timeout occurred
    virtual void reset() = 0;

    //! @returns function code (mask) for this peripheral function
    uint32_t getFunctionCode() { return mFunctionCode; }

    //! @returns the function definition for this peripheral function
    virtual uint32_t getFunctionDefinition() = 0;

private:
    DreamcastPeripheralFunction() = delete;

protected:
    //! The function code (mask) for this peripheral function
    const uint32_t mFunctionCode;
};

}
