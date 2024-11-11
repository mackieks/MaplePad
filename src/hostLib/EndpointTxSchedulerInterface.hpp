#pragma once

#include "hal/MapleBus/MaplePacket.hpp"
#include "dreamcast_constants.h"
#include "Transmitter.hpp"

class EndpointTxSchedulerInterface
{
public:
    //! Default constructor
    EndpointTxSchedulerInterface() {}

    //! Virtual destructor
    virtual ~EndpointTxSchedulerInterface() {}

    //! Add a transmission to the schedule
    //! @param[in] txTime  Time at which this should transmit in microseconds
    //! @param[in] transmitter  Pointer to transmitter that is adding this
    //! @param[in] command  The command to send
    //! @param[in] payload  The payload of the above command
    //! @param[in] payloadLen  The length of the above payload
    //! @param[in] expectResponse  true iff a response is expected after transmission
    //! @param[in] expectedResponseNumPayloadWords  Number of payload words to expect in response
    //! @param[in] autoRepeatUs  How often to repeat this transmission in microseconds
    //! @param[in] autoRepeatEndTimeUs  If not 0, auto repeat will cancel after this time
    //! @returns transmission ID
    virtual uint32_t add(uint64_t txTime,
                         Transmitter* transmitter,
                         uint8_t command,
                         uint32_t* payload,
                         uint8_t payloadLen,
                         bool expectResponse,
                         uint32_t expectedResponseNumPayloadWords=0,
                         uint32_t autoRepeatUs=0,
                         uint64_t autoRepeatEndTimeUs=0) = 0;

    //! Cancels scheduled transmission by transmission ID
    //! @param[in] transmissionId  The transmission ID of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    virtual uint32_t cancelById(uint32_t transmissionId) = 0;

    //! Cancels scheduled transmission by recipient address
    //! @param[in] recipientAddr  The recipient address of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    virtual uint32_t cancelByRecipient(uint8_t recipientAddr) = 0;

    //! Count how many scheduled transmissions have a given recipient address
    //! @param[in] recipientAddr  The recipient address
    //! @returns the number of transmissions have the given recipient address
    virtual uint32_t countRecipients(uint8_t recipientAddr) = 0;

    //! Cancels all items in the schedule
    //! @returns number of transmissions successfully canceled
    virtual uint32_t cancelAll() = 0;
};