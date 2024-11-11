#pragma once

#include "hal/MapleBus/MaplePacket.hpp"
#include "dreamcast_constants.h"
#include "Transmission.hpp"
#include <list>
#include <vector>
#include <memory>

class PrioritizedTxScheduler
{
public:
    //! Enumerates available priorities to be used with add()
    enum Priority : uint8_t
    {
        //! Priority for external entity taking control of the bus (max)
        EXTERNAL_TRANSMISSION_PRIORITY = 0,
        //! Priority for main peripheral
        MAIN_TRANSMISSION_PRIORITY,
        //! Priority for sub peripheral (min)
        SUB_TRANSMISSION_PRIORITY,
        //! Any selected priority must be less than PRIORITY_COUNT
        PRIORITY_COUNT
    };

    //! Points to a schedule item within the current schedule
    class ScheduleItem
    {
        //! Only the PrioritizedTxScheduler may modify private elements here
        friend PrioritizedTxScheduler;

        public:
            //! Constructor
            ScheduleItem() : mIsValid(false), mTime(0) {}

            //! @returns the transmission for this schedule item
            std::shared_ptr<Transmission> getTx() {return mIsValid ? *mItemIter : nullptr;}

        private:
            //! Set to true iff iterators are valid
            bool mIsValid;
            //! The schedule group
            std::vector<std::list<std::shared_ptr<Transmission>>>::iterator mScheduleIter;
            //! The item within the schedule group
            std::list<std::shared_ptr<Transmission>>::iterator mItemIter;
            //! The time at which this item was peeked
            uint64_t mTime;
    };

public:
    //! Default constructor
    //! @param[in] senderAddress  The sender address set in every packet added
    //! @param[in] max  The maximum accepted priority
    PrioritizedTxScheduler(uint8_t senderAddress, uint32_t max = (PRIORITY_COUNT-1));

    //! Virtual destructor
    virtual ~PrioritizedTxScheduler();

    //! Add a transmission to the schedule
    //! @param[in] priority  priority of this transmission (0 is highest priority)
    //! @param[in] txTime  Time at which this should transmit in microseconds
    //! @param[in] transmitter  Pointer to transmitter that is adding this
    //! @param[in,out] packet  Packet data to send (internal data is moved upon calling this)
    //! @param[in] expectResponse  true iff a response is expected after transmission
    //! @param[in] expectedResponseNumPayloadWords  Number of payload words to expect in response
    //! @param[in] autoRepeatUs  How often to repeat this transmission in microseconds
    //! @param[in] autoRepeatEndTimeUs  If not 0, auto repeat will cancel after this time
    //! @returns transmission ID
    uint32_t add(uint8_t priority,
                 uint64_t txTime,
                 Transmitter* transmitter,
                 MaplePacket& packet,
                 bool expectResponse,
                 uint32_t expectedResponseNumPayloadWords=0,
                 uint32_t autoRepeatUs=0,
                 uint64_t autoRepeatEndTimeUs=0);

    //! Peeks the next scheduled packet, given the current time
    //! @param[in] time  The current time
    //! @returns nullptr if no scheduled packet is available for the given time
    //! @returns the next scheduled item for the given current time
    ScheduleItem peekNext(uint64_t time);

    //! Pops a schedule item that was retrieved using peekNext
    //! @param[in,out] scheduleItem  The schedule item to pop and invalidate
    std::shared_ptr<Transmission> popItem(ScheduleItem& scheduleItem);

    //! Cancels scheduled transmission by transmission ID
    //! @param[in] transmissionId  The transmission ID of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    uint32_t cancelById(uint32_t transmissionId);

    //! Cancels scheduled transmission by recipient address
    //! @param[in] recipientAddr  The recipient address of the transmissions to cancel
    //! @returns number of transmissions successfully canceled
    uint32_t cancelByRecipient(uint8_t recipientAddr);

    //! Count how many scheduled transmissions have a given recipient address
    //! @param[in] recipientAddr  The recipient address
    //! @returns the number of transmissions have the given recipient address
    uint32_t countRecipients(uint8_t recipientAddr);

    //! Cancels all items in the schedule
    //! @returns number of transmissions successfully canceled
    uint32_t cancelAll();

    //! Computes the next time on a cadence
    //! @param[in] currentTime  The current time
    //! @param[in] period  The period at which this item is scheduled (must be > 0)
    //! @param[in] offset  The offset that this item began or previously executed at
    //! @returns the next time in the future which is confined to period and offset
    static uint64_t computeNextTimeCadence(uint64_t currentTime,
                                           uint64_t period,
                                           uint64_t offset = 0);

protected:
    //! Add a transmission to the schedule
    //! @param[in] tx  The transmission to add
    //! @returns transmission ID
    uint32_t add(std::shared_ptr<Transmission> tx);

public:
    //! Use this for txTime if the packet needs to be sent ASAP
    static const uint64_t TX_TIME_ASAP = 0;
    //! Transmission ID to use in order to flag no ID
    static const uint32_t INVALID_TX_ID = 0;

protected:
    //! The address of this sender
    const uint8_t mSenderAddress;
    //! The next transmission ID to set
    uint32_t mNextId;
    //! The current schedule ordered by priority and time
    std::vector<std::list<std::shared_ptr<Transmission>>> mSchedule;
};