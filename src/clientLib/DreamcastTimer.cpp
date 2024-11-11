#include "DreamcastTimer.hpp"

#include <ctime>

namespace client
{
DreamcastTimer::DreamcastTimer(const ClockInterface& clock, SetTimeFn setTimeFn, SetPwmFn setPwmFn) :
    DreamcastPeripheralFunction(DEVICE_FN_TIMER),
    mClock(clock),
    mSetTimeFn(setTimeFn),
    mSetPwmFn(setPwmFn),
    mSetTime{
        .baseTime = 0,
        .dateTime = {
            .year = 1999,
            .month = 9,
            .day = 9,
            .hour = 0,
            .minute = 0,
            .second = 0
        }
    }
{}

bool DreamcastTimer::handlePacket(const MaplePacket& in, MaplePacket& out)
{
    const uint8_t cmd = in.frame.command;
    switch (cmd)
    {
        case COMMAND_BLOCK_WRITE:
        {
            if (in.payload.size() >= 4 && in.payload[1] == 0)
            {
                // Set date/time
                mSetTime.baseTime = mClock.getTimeUs();
                DateTime& dateTime = mSetTime.dateTime;
                dateTime.year = in.payload[2] >> 16;
                dateTime.month = (in.payload[2] >> 8) & 0xFF;
                dateTime.day = in.payload[2] & 0xFF;
                dateTime.hour = in.payload[3] >> 24;
                dateTime.minute = (in.payload[3] >> 16) & 0xFF;
                dateTime.second = (in.payload[3] >> 8) & 0xFF;
                dateTime.dayOfWeek = in.payload[3] & 0xFF;

                if (mSetTimeFn)
                {
                    mSetTimeFn(mSetTime);
                }

                out.frame.command = COMMAND_RESPONSE_ACK;
                return true;
            }
        }
        break;

        case COMMAND_BLOCK_READ:
        {
            if (in.payload.size() >= 2 && in.payload[1] == 0)
            {
                DateTime dateTime = getCurrentDateTime();
                out.frame.command = COMMAND_RESPONSE_DATA_XFER;
                out.reservePayload(3);
                out.appendPayload(getFunctionCode());
                out.appendPayload(dateTime.year << 16 | dateTime.month << 8 | dateTime.day);
                out.appendPayload(dateTime.hour << 24 | dateTime.minute << 16 | dateTime.second << 8 | dateTime.dayOfWeek);
                return true;
            }
        }
        break;

        case COMMAND_GET_CONDITION:
        {
            out.frame.command = COMMAND_RESPONSE_DATA_XFER;
            out.reservePayload(2);
            out.appendPayload(getFunctionCode());
            out.appendPayload(0xFF000000);
            return true;
        }
        break;

        case COMMAND_SET_CONDITION:
        {
            if (in.payload.size() >= 2)
            {
                if (mSetPwmFn)
                {
                    uint8_t width = (in.payload[1] >> 24) & 0xFF;
                    uint8_t down = (in.payload[1] >> 16) & 0xFF;
                    mSetPwmFn(width, down);
                }
                out.frame.command = COMMAND_RESPONSE_ACK;
                return true;
            }
        }
        break;

        default:
        {
        }
        break;
    }

    return false;

}

void DreamcastTimer::reset()
{
    if (mSetPwmFn)
    {
        // Alarm is shut off on reset
        mSetPwmFn(0, 0);
    }
}

uint32_t DreamcastTimer::getFunctionDefinition()
{
    return 0x7E7E3F40;
}

DreamcastTimer::DateTime DreamcastTimer::getCurrentDateTime()
{
    std::tm t = {};
    t.tm_year = mSetTime.dateTime.year - 1900;
    t.tm_mon = mSetTime.dateTime.month - 1;
    t.tm_mday = mSetTime.dateTime.day;
    t.tm_hour = mSetTime.dateTime.hour;
    t.tm_min = mSetTime.dateTime.minute;
    t.tm_sec = mSetTime.dateTime.second;
    std::time_t epoch = std::mktime(&t);

    uint64_t clockDiff = mClock.getTimeUs() - mSetTime.baseTime;

    epoch += (clockDiff / 1000000);

    t = *std::gmtime(&epoch);

    DateTime dateTime = {
        .year = static_cast<uint16_t>(t.tm_year + 1900),
        .month = static_cast<uint8_t>(t.tm_mon + 1),
        .day = static_cast<uint8_t>(t.tm_mday),
        .hour = static_cast<uint8_t>(t.tm_hour),
        .minute = static_cast<uint8_t>(t.tm_min),
        .second = static_cast<uint8_t>(t.tm_sec),
        .dayOfWeek = static_cast<uint8_t>((t.tm_wday + 6) % 7)
    };

    return dateTime;
}

}
