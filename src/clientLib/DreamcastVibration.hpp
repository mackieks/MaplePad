#pragma once

#include <memory>
#include <vector>
#include <bitset>
#include <assert.h>

#include "DreamcastPeripheralFunction.hpp"
#include "VibrationObserver.hpp"

namespace client
{

class DreamcastVibration: public DreamcastPeripheralFunction
{
public:
    //! Constructor
    inline DreamcastVibration() :
        DreamcastPeripheralFunction(DEVICE_FN_VIBRATION),
        mVibrationObserver(nullptr),
        mLastCondition(0x10000000),
        mAutoStopValue(0x13)
    {}

    //! Handle packet meant for this peripheral function
    //! @param[in] in  The packet read from the Maple Bus
    //! @param[out] out  The packet to write to the Maple Bus when true is returned
    //! @returns true iff the packet was handled
    inline virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final
    {
        switch (in.frame.command)
        {
            case COMMAND_GET_MEMORY_INFORMATION:
            {
                if (in.payload.size() > 1)
                {
                    uint8_t source = in.payload[1] >> 24 & 0xFF;
                    // Only 1 source is available for this function
                    if (source == 1)
                    {
                        out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                        out.reservePayload(2);
                        out.appendPayload(getFunctionCode());
                        // Same as OEM puru puru pack - no arbitrary waveform support
                        out.appendPayload(0x10E0073B);
                        return true;
                    }
                    else
                    {
                        out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                        return true;
                    }
                }
                else
                {
                    out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                    return true;
                }
            }
            break;

            case COMMAND_SET_CONDITION:
            {
                if (in.payload.size() > 1)
                {
                    uint8_t ctrl = in.payload[1] >> 24 & 0xFF;
                    uint8_t source = ctrl >> 4;
                    // Only 1 source is available for this function
                    if (source == 1)
                    {
                        if (mVibrationObserver != nullptr)
                        {
                            bool isContinuous = ((ctrl & 0x01) != 0);
                            uint8_t pow = in.payload[1] >> 16 & 0xFF;
                            uint8_t freq = in.payload[1] >> 8 & 0xFF;
                            uint8_t cycles = in.payload[1] & 0xFF;

                            int8_t inclination = 0;
                            uint8_t pow3 = (pow >> 4) & 0x07; // 3-bit power
                            if ((pow & 0x08) != 0)
                            {
                                inclination = 1;
                            }
                            else if ((pow & 0x80) != 0)
                            {
                                inclination = -1;
                                pow3 = pow & 0x07;
                            }
                            else if (pow3 == 0)
                            {
                                // Just in case power is actually stored here instead?
                                pow3 = pow & 0x07;
                            }

                            float frequency = (freq + 1) / 2.0;
                            float intensity = pow3 / 7.0;
                            float duration = (1 / frequency) * (cycles + 1);
                            if (isContinuous)
                            {
                                // Continuous duration
                                duration = mAutoStopValue * 0.25 + 0.25;
                            }
                            else if (inclination < 0)
                            {
                                // Multiply by each power value in the inclination for total duration
                                duration *= pow3;
                            }
                            else if (inclination > 0)
                            {
                                // Multiply by each power value upt to max for total duration
                                duration *= (8 - pow3);
                            }

                            // Send it!
                            mVibrationObserver->vibrate(frequency, intensity, inclination, duration);
                        }

                        out.frame.command = COMMAND_RESPONSE_ACK;
                        return true;
                    }
                    else
                    {
                        out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                        return true;
                    }
                }
                else
                {
                    out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                    return true;
                }
            }
            break;

            case COMMAND_GET_CONDITION:
            {
                out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                out.reservePayload(2);
                out.appendPayload(getFunctionCode());
                out.appendPayload(mLastCondition);
                return true;
            }
            break;

            case COMMAND_BLOCK_WRITE:
            {
                if (in.payload.size() > 1)
                {
                    uint8_t vn = in.payload[1] >> 24 & 0xFF;
                    if (vn == 0)
                    {
                        if (in.payload.size() > 2)
                        {
                            // Set auto-stop time
                            mAutoStopValue = in.payload[2] >> 8 & 0xFF;
                        }
                        else
                        {
                            out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                            return true;
                        }
                    }
                    else
                    {
                        // Arbitrary waveform not supported
                        out.frame.command = COMMAND_RESPONSE_UNKNOWN_COMMAND;
                        return true;
                    }
                }
                else
                {
                    out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                    return true;
                }
            }
            break;

            case COMMAND_BLOCK_READ:
            {
                if (in.payload.size() > 1)
                {
                    uint8_t vn = in.payload[1] >> 24 & 0xFF;
                    if (vn == 0)
                    {
                        out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                        out.reservePayload(3);
                        out.appendPayload(getFunctionCode());
                        out.appendPayload(0);
                        out.appendPayload(0x00020000 | (static_cast<uint32_t>(mAutoStopValue) << 8));
                        return true;
                    }
                    else
                    {
                        // Arbitrary waveform not supported
                        out.frame.command = COMMAND_RESPONSE_UNKNOWN_COMMAND;
                        return true;
                    }
                }
                else
                {
                    out.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                    return true;
                }
            }
            break;

            default:
                // Ignore
                break;
        }

        return false;
    }

    //! Called when player index changed or timeout occurred
    inline virtual void reset() final
    {}

    //! @returns the function definition for this peripheral function
    inline virtual uint32_t getFunctionDefinition() final
    {
        // 1 total vibration source
        // 1 vibration source that can be used
        return 0x01010000;
    }

    //! Sets the observer which will receive vibrations
    inline void setObserver(VibrationObserver* vibrationObserver)
    {
        mVibrationObserver = vibrationObserver;
    }

private:
    //! Pointer to the vibration observer receiving vibrations
    VibrationObserver* mVibrationObserver;
    //! The last set condition
    uint32_t mLastCondition;
    //! Specifies time for auto stop (continuous)
    uint8_t mAutoStopValue;
};

}
