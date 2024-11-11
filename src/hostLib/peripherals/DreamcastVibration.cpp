#include "DreamcastVibration.hpp"
#include "utils.h"
#include "global_constants.h"
#include <algorithm>

// I generated these equations based on observed vibrations with two test devices:
// Dreamcast OEM Jump Pack AKA Puru Puru Pack AKA Vibration Pack
// Performance TremorPak

// Converts pulsation frequency value into approximate pulsation frequency in Hz
#define PULSATION_FREQ(freqValue) ((freqValue + 1) * 0.5)

// Computes cycle value for given increments, frequency, and duration
#define COMPUTE_CYCLES(numIncrements, freqValue, durationMs) (                                      \
    (uint8_t)limit_value(                                                                           \
        (int32_t)((PULSATION_FREQ(freqValue) * (durationMs / (double)MILLISECONDS_PER_SECOND))      \
                  / numIncrements - 1 + 0.5),                                                       \
        (int32_t)MIN_CYCLES,                                                                        \
        (int32_t)MAX_CYCLES)                                                                        \
)

// Computes a single increment duration given frequency and cycles values
#define COMPUTE_DURATION(freqValue, cyclesValue, multiplier) (multiplier * (cyclesValue + 1) / PULSATION_FREQ(freqValue))
#define COMPUTE_DURATION_US(freqValue, cyclesValue) COMPUTE_DURATION(freqValue, cyclesValue, MICROSECONDS_PER_SECOND)
#define COMPUTE_DURATION_MS(freqValue, cyclesValue) COMPUTE_DURATION(freqValue, cyclesValue, MILLISECONDS_PER_SECOND)

// Frequency index in MAX_DURATION_MS_LOOKUP to frequency value
#define FREQ_IDX_TO_FREQ(freqIdx) (freqIdx + MIN_FREQ_VALUE)

// Maximum duration for a single increment
#define MAX_DURATION_MS_IDX(freqIdx) (uint32_t)COMPUTE_DURATION_MS(FREQ_IDX_TO_FREQ(freqIdx), MAX_CYCLES)

// There are ways of generating this like using BOOST_PP_ENUM, but I don't want to include boost just for that
const uint32_t DreamcastVibration::MAX_DURATION_MS_LOOKUP[NUM_FREQ_VALUES] =
{
    MAX_DURATION_MS_IDX(0),  MAX_DURATION_MS_IDX(1),  MAX_DURATION_MS_IDX(2),  MAX_DURATION_MS_IDX(3),
    MAX_DURATION_MS_IDX(4),  MAX_DURATION_MS_IDX(5),  MAX_DURATION_MS_IDX(6),  MAX_DURATION_MS_IDX(7),
    MAX_DURATION_MS_IDX(8),  MAX_DURATION_MS_IDX(9),  MAX_DURATION_MS_IDX(10), MAX_DURATION_MS_IDX(11),
    MAX_DURATION_MS_IDX(12), MAX_DURATION_MS_IDX(13), MAX_DURATION_MS_IDX(14), MAX_DURATION_MS_IDX(15),
    MAX_DURATION_MS_IDX(16), MAX_DURATION_MS_IDX(17), MAX_DURATION_MS_IDX(18), MAX_DURATION_MS_IDX(19),
    MAX_DURATION_MS_IDX(20), MAX_DURATION_MS_IDX(21), MAX_DURATION_MS_IDX(22), MAX_DURATION_MS_IDX(23),
    MAX_DURATION_MS_IDX(24), MAX_DURATION_MS_IDX(25), MAX_DURATION_MS_IDX(26), MAX_DURATION_MS_IDX(27),
    MAX_DURATION_MS_IDX(28), MAX_DURATION_MS_IDX(29), MAX_DURATION_MS_IDX(30), MAX_DURATION_MS_IDX(31),
    MAX_DURATION_MS_IDX(32), MAX_DURATION_MS_IDX(33), MAX_DURATION_MS_IDX(34), MAX_DURATION_MS_IDX(35),
    MAX_DURATION_MS_IDX(36), MAX_DURATION_MS_IDX(37), MAX_DURATION_MS_IDX(38), MAX_DURATION_MS_IDX(39),
    MAX_DURATION_MS_IDX(40), MAX_DURATION_MS_IDX(41), MAX_DURATION_MS_IDX(42), MAX_DURATION_MS_IDX(43),
    MAX_DURATION_MS_IDX(44), MAX_DURATION_MS_IDX(45), MAX_DURATION_MS_IDX(46), MAX_DURATION_MS_IDX(47),
    MAX_DURATION_MS_IDX(48), MAX_DURATION_MS_IDX(49), MAX_DURATION_MS_IDX(50), MAX_DURATION_MS_IDX(51),
    MAX_DURATION_MS_IDX(52)
};

DreamcastVibration::DreamcastVibration(uint8_t addr,
                                       uint32_t fd,
                                       std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                       PlayerData playerData) :
    DreamcastPeripheral("vibration", addr, fd, scheduler, playerData.playerIndex),
    mTransmissionId(0),
    mFirst(true)
{
}

DreamcastVibration::~DreamcastVibration()
{}

void DreamcastVibration::task(uint64_t currentTimeUs)
{
    if (mFirst)
    {
        mFirst = false;

        // Send some vibrations on connection
        send(currentTimeUs, 7, 0, 0, 500);
    }
}

void DreamcastVibration::txStarted(std::shared_ptr<const Transmission> tx)
{
    if (tx->transmissionId == mTransmissionId)
    {
        mTransmissionId = 0;
    }
}

void DreamcastVibration::txFailed(bool writeFailed,
                                  bool readFailed,
                                  std::shared_ptr<const Transmission> tx)
{
}

void DreamcastVibration::txComplete(std::shared_ptr<const MaplePacket> packet,
                                    std::shared_ptr<const Transmission> tx)
{
}

uint8_t DreamcastVibration::computeNumIncrements(uint8_t power, int8_t inclination)
{
    // Compute the number of increments to execute
    if (inclination > 0)
    {
        // Increase from given power to max power
        return (MAX_POWER - power + 1);
    }
    else if (inclination < 0)
    {
        // Decrease from given power to min power
        return  (power - MIN_POWER + 1);
    }
    else
    {
        return 1;
    }
}

uint8_t DreamcastVibration::maxFreqForDuration(uint8_t numIncrements, uint32_t durationMs)
{
    // Compute the maximum frequency in order to achieve the given duration
    for (uint32_t i = NUM_FREQ_VALUES; i > 0; --i)
    {
        if (durationMs <= (MAX_DURATION_MS_LOOKUP[i - 1] * numIncrements))
        {
            return FREQ_IDX_TO_FREQ(i - 1);
        }
    }

    // Can't achieve this duration, so just return min to get the most out of it
    return MIN_FREQ_VALUE;
}

void DreamcastVibration::send(uint64_t timeUs, uint8_t power, int8_t inclination, uint8_t desiredFreq, uint32_t durationMs)
{
    // Vibration set-condition command second payload word:
    // Byte 0: cycles (00 to FF)
    //         - The number of pulsation cycles per ramp increment
    //         - Value of 00 is not valid when ramp up/down set
    //         - Dreamcast OEM ignores this value when ramp up/down not set (just pulses)
    // Byte 1: pulsation frequency value (07 to 3B)
    //         - Lower values cause noticeable pulsation
    //         - The lower the value, the longer the total vibration duration
    //         - The frequency seems to correlate to about (value / 2 + 1) Hz
    //         - For Dreamcast OEM: this value makes no real difference when power is 7
    // Byte 2: intensity/ramp up/ramp down
    //         - Each intensity is executed for the number of specified cycles
    //         - When value is 0xX0 where X is:
    //            - 0-7 : Single stable vibration (0: off, 1: low, 7: high)
    //         - When value is 0xX8 where X is:
    //            - 0-7 : Ramp up, starting intensity up to max (0: off, 1: low, 7: high)
    //         - When value is 0x8X where X is:
    //            - 0-7 : Ramp down, starting intensity down to min (0: off, 1: low, 7: high)
    // Byte 3: 10 or 11
    //         - Most sig nibble must be 1 for the command to be accepted
    //         - Least sig nibble must be 0 or 1 for the command to be accepted
    //         - The least significant nibble when set to 1: augments total duration
    //            - Not going to support this since I can't find a game which used this bit
    // A value of 0x10000000 will stop current vibration
    uint32_t vibrationWord = 0x10000000;

    // Default: don't automatically repeat this transmission
    uint32_t autoRepeatUs = 0;
    uint64_t autoRepeatEndTimeUs = 0;

    if (power > 0)
    {
        // Limit power value to valid value
        power = limit_value(power, MIN_POWER, MAX_POWER);

        // Compute number of increments and frequency
        uint8_t numIncrements = computeNumIncrements(power, inclination);
        uint8_t freq = MAX_FREQ_VALUE;
        if (desiredFreq == 0)
        {
            if (inclination != 0)
            {
                // Get the maximum frequency that can achieve the given duration
                freq = maxFreqForDuration(numIncrements, durationMs);
            }
            // else: default to MAX_FREQ_VALUE
        }
        else
        {
            // Just make sure the value fits within limits
            freq = limit_value(desiredFreq, MIN_FREQ_VALUE, MAX_FREQ_VALUE);
        }

        // Set Power and inclination direction into word
        if (inclination > 0)
        {
            vibrationWord |= (power << 20) | 0x080000;
        }
        else if (inclination < 0)
        {
            vibrationWord |= (power << 16) | 0x800000;
        }
        else
        {
            vibrationWord |= (power << 20);
        }

        // Compute number of pulse cycles per increment
        uint8_t cycles = 0;
        if (inclination != 0)
        {
            cycles = COMPUTE_CYCLES(numIncrements, freq, durationMs);
            // Command is invalid if cycles set to 0 when inclination is also set
            if (cycles == 0)
            {
                cycles = 1;
            }
        }
        else
        {
            // Dreamcast OEM jump packs ignore this value when inclination is not set.
            // Instead, automatically repeat the pulse until selected duration has elapsed.
            cycles = 0;

            double cycleDurationUs = COMPUTE_DURATION_US(freq, 0);
            if (durationMs * MICROSECONDS_PER_MILLISECOND > cycleDurationUs)
            {
                // Automatically resend at half the duration
                autoRepeatUs = cycleDurationUs * 0.5;
                // The time when the final vibration should be sent
                autoRepeatEndTimeUs =
                    timeUs + (durationMs * MICROSECONDS_PER_MILLISECOND) - cycleDurationUs + 1;
            }
            // else: send single pulse
        }

        // Set frequency and duration
        vibrationWord |= (freq << 8) | cycles;
    }
    // else: send "stop" command

    // Remove past transmission if it hasn't been sent yet
    if (mTransmissionId > 0)
    {
        mEndpointTxScheduler->cancelById(mTransmissionId);
        mTransmissionId = 0;
    }

    // Send it!
    uint32_t payload[2] = {FUNCTION_CODE, vibrationWord};
    mTransmissionId = mEndpointTxScheduler->add(
        timeUs,
        this,
        COMMAND_SET_CONDITION,
        payload,
        2,
        true,
        0,
        autoRepeatUs,
        autoRepeatEndTimeUs);
}

void DreamcastVibration::start(uint8_t power, uint8_t desiredFreq)
{
    uint8_t freq = MAX_FREQ_VALUE;
    if (desiredFreq != 0)
    {
        freq = limit_value(desiredFreq, MIN_FREQ_VALUE, MAX_FREQ_VALUE);
    }
    // else: default to MAX_FREQ_VALUE

    uint32_t vibrationWord = 0x10000000 | (power << 20) | (freq << 8);

    // Automatically repeat at half the duration
    uint32_t autoRepeatUs = COMPUTE_DURATION_US(freq, 0) * 0.5;

    // Remove past transmission if it hasn't been sent yet
    if (mTransmissionId > 0)
    {
        mEndpointTxScheduler->cancelById(mTransmissionId);
        mTransmissionId = 0;
    }

    // Send it!
    uint32_t payload[2] = {FUNCTION_CODE, vibrationWord};
    mTransmissionId = mEndpointTxScheduler->add(
        PrioritizedTxScheduler::TX_TIME_ASAP,
        this,
        COMMAND_SET_CONDITION,
        payload,
        2,
        true,
        0,
        autoRepeatUs);
}

void DreamcastVibration::stop()
{
    send(PrioritizedTxScheduler::TX_TIME_ASAP, 0, 0, 0, 0);
}
