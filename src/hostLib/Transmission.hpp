#pragma once

#include <stdint.h>
#include <memory>
#include "hal/MapleBus/MaplePacket.hpp"
#include "Transmitter.hpp"

//! Transmission definition
struct Transmission
{
    //! Unique ID of this transmission
    const uint32_t transmissionId;
    //! Priority where 0 is highest
    const uint8_t priority;
    //! Set to true iff a response is expected
    const bool expectResponse;
    //! The expected transmission duration (from transmit to end of receive)
    const uint32_t txDurationUs;
    //! If not 0, the period which this transmission should be repeated in microseconds
    const uint32_t autoRepeatUs;
    //! If not 0, auto repeat will cancel after this time
    const uint64_t autoRepeatEndTimeUs;
    //! The next time that this packet is to be transmitted
    uint64_t nextTxTimeUs;
    //! The packet to transmit
    std::shared_ptr<const MaplePacket> packet;
    //! The object that added this transmission (for callbacks)
    Transmitter* const transmitter;

    Transmission(uint32_t transmissionId,
                 uint8_t priority,
                 bool expectResponse,
                 uint32_t txDurationUs,
                 uint32_t autoRepeatUs,
                 uint64_t autoRepeatEndTimeUs,
                 uint64_t nextTxTimeUs,
                 std::shared_ptr<MaplePacket> packet,
                 Transmitter* transmitter):
        transmissionId(transmissionId),
        priority(priority),
        expectResponse(expectResponse),
        txDurationUs(txDurationUs),
        autoRepeatUs(autoRepeatUs),
        autoRepeatEndTimeUs(autoRepeatEndTimeUs),
        nextTxTimeUs(nextTxTimeUs),
        packet(packet),
        transmitter(transmitter)
    {}

    //! @returns the estimated completion time of this transmission
    uint64_t getNextCompletionTime(uint64_t executionTime)
    {
        return executionTime + txDurationUs;
    }
};