#include "PrioritizedTxScheduler.hpp"
#include "configuration.h"
#include "utils.h"

#include <assert.h>

// STL
#include <algorithm>

PrioritizedTxScheduler::PrioritizedTxScheduler(uint8_t senderAddress, uint32_t max) :
    mSenderAddress(senderAddress),
    mNextId(1),
    mSchedule()
{
    mSchedule.resize(max + 1);
}

PrioritizedTxScheduler::~PrioritizedTxScheduler() {}

uint32_t PrioritizedTxScheduler::add(std::shared_ptr<Transmission> tx)
{
    assert(tx->priority < mSchedule.size());
    std::list<std::shared_ptr<Transmission>>& schedule = mSchedule[tx->priority];
    // Keep iterating until correct position is found
    std::list<std::shared_ptr<Transmission>>::const_iterator iter = schedule.cbegin();
    while(iter != schedule.cend() && tx->nextTxTimeUs >= (*iter)->nextTxTimeUs)
    {
        ++iter;
    }
    schedule.insert(iter, tx);
    return tx->transmissionId;
}

uint32_t PrioritizedTxScheduler::add(uint8_t priority,
                                    uint64_t txTime,
                                    Transmitter* transmitter,
                                    MaplePacket& packet,
                                    bool expectResponse,
                                    uint32_t expectedResponseNumPayloadWords,
                                    uint32_t autoRepeatUs,
                                    uint64_t autoRepeatEndTimeUs)
{
    uint32_t pktDurationNs = MAPLE_OPEN_LINE_CHECK_TIME_US + packet.getTxTimeNs();

    if (expectResponse)
    {
        uint32_t expectedReadDurationUs = MaplePacket::getTxTimeNs(expectedResponseNumPayloadWords, MAPLE_RESPONSE_NS_PER_BIT);
        pktDurationNs += MAPLE_RESPONSE_DELAY_NS + expectedReadDurationUs;
    }

    uint32_t pktDurationUs = INT_DIVIDE_CEILING(pktDurationNs, 1000);

    // This will happen if minimal communication is made constantly for 20 days
    assert(mNextId != INVALID_TX_ID);

    // Update the sender address to my address
    packet.frame.senderAddr = mSenderAddress;

    std::shared_ptr<Transmission> tx =
        std::make_shared<Transmission>(mNextId++,
                                       priority,
                                       expectResponse,
                                       pktDurationUs,
                                       autoRepeatUs,
                                       autoRepeatEndTimeUs,
                                       txTime,
                                       std::make_shared<MaplePacket>(std::move(packet)),
                                       transmitter);

    return add(tx);
}

uint64_t PrioritizedTxScheduler::computeNextTimeCadence(uint64_t currentTime,
                                                        uint64_t period,
                                                        uint64_t offset)
{
    // Cover the edge case where the offset is in the future for some reason
    if (offset > currentTime)
    {
        return offset;
    }
    else
    {
        // Determine how many intervals to advance past offset
        uint32_t n = INT_DIVIDE_CEILING(currentTime - offset, period);
        if (n == 0)
        {
            n = 1;
        }
        // Compute the next cadence
        return offset + (period * n);
    }
}

PrioritizedTxScheduler::ScheduleItem PrioritizedTxScheduler::peekNext(uint64_t time)
{
    ScheduleItem scheduleItem;

    // Find a priority list with item ready to be popped
    std::vector<std::list<std::shared_ptr<Transmission>>>::iterator scheduleIter = mSchedule.begin();
    while (scheduleIter != mSchedule.end()
           && (scheduleIter->empty() || (*scheduleIter->begin())->nextTxTimeUs > time))
    {
        ++scheduleIter;
    }

    if (scheduleIter != mSchedule.end())
    {
        std::list<std::shared_ptr<Transmission>>::iterator itemIter = scheduleIter->begin();

        bool found = true;
        if (scheduleIter != mSchedule.begin())
        {
            std::list<uint8_t> recipients;
            found = false;
            do
            {
                // Something was found, so make sure it won't be executing while something of higher
                // priority is scheduled to run
                uint64_t completionTime = (*itemIter)->getNextCompletionTime(time);
                uint8_t recipientAddr = (*itemIter)->packet->frame.recipientAddr;

                // Preserve order for each recipient
                // (don't use this if we already skipped one for the same recipient)
                if (std::find(recipients.begin(), recipients.end(), recipientAddr) == recipients.end())
                {
                    found = true;
                    std::vector<std::list<std::shared_ptr<Transmission>>>::iterator scheduleIter2 = scheduleIter;
                    do
                    {
                        --scheduleIter2;
                        if (!scheduleIter2->empty() && (*scheduleIter2->begin())->nextTxTimeUs < completionTime)
                        {
                            // Something found - don't use this lower priority item
                            found = false;
                            break;
                        }
                    } while (scheduleIter2 != mSchedule.begin());
                }

                if (!found)
                {
                    recipients.push_back(recipientAddr);
                    ++itemIter;
                }
                else
                {
                    break;
                }
            } while (itemIter != scheduleIter->end() && (*itemIter)->nextTxTimeUs <= time);
        }

        if (found)
        {
            scheduleItem.mScheduleIter = scheduleIter;
            scheduleItem.mItemIter = itemIter;
            scheduleItem.mTime = time;
            scheduleItem.mIsValid = true;
        }
    }

    return scheduleItem;
}

std::shared_ptr<Transmission> PrioritizedTxScheduler::popItem(ScheduleItem& scheduleItem)
{
    std::shared_ptr<Transmission> item = nullptr;

    if (scheduleItem.mIsValid)
    {
        // Save the transmission
        item = scheduleItem.getTx();

        // Pop it!
        scheduleItem.mScheduleIter->erase(scheduleItem.mItemIter);
        scheduleItem.mIsValid = false;

        // Reschedule this if auto repeat settings are valid
        if (item != nullptr
            && item->autoRepeatUs > 0
            && (item->autoRepeatEndTimeUs == 0 || scheduleItem.mTime <= item->autoRepeatEndTimeUs))
        {
            item->nextTxTimeUs = computeNextTimeCadence(scheduleItem.mTime,
                                                        item->autoRepeatUs,
                                                        item->nextTxTimeUs);
            add(item);
        }
    }

    return item;
}

uint32_t PrioritizedTxScheduler::cancelById(uint32_t transmissionId)
{
    uint32_t n = 0;
    for (std::vector<std::list<std::shared_ptr<Transmission>>>::iterator scheduleIter = mSchedule.begin();
         scheduleIter != mSchedule.end();
         ++scheduleIter)
    {
        std::list<std::shared_ptr<Transmission>>::iterator iter2 = scheduleIter->begin();
        while (iter2 != scheduleIter->end())
        {
            if ((*iter2)->transmissionId == transmissionId)
            {
                iter2 = scheduleIter->erase(iter2);
                ++n;
            }
            else
            {
                ++iter2;
            }
        }
    }

    return n;
}

uint32_t PrioritizedTxScheduler::cancelByRecipient(uint8_t recipientAddr)
{
    uint32_t n = 0;
    for (std::vector<std::list<std::shared_ptr<Transmission>>>::iterator scheduleIter = mSchedule.begin();
         scheduleIter != mSchedule.end();
         ++scheduleIter)
    {
        std::list<std::shared_ptr<Transmission>>::iterator iter = scheduleIter->begin();
        while (iter != scheduleIter->end())
        {
            if ((*iter)->packet->frame.recipientAddr == recipientAddr)
            {
                iter = scheduleIter->erase(iter);
                ++n;
            }
            else
            {
                ++iter;
            }
        }
    }
    return n;
}

uint32_t PrioritizedTxScheduler::countRecipients(uint8_t recipientAddr)
{
    uint32_t n = 0;
    for (std::vector<std::list<std::shared_ptr<Transmission>>>::iterator scheduleIter = mSchedule.begin();
         scheduleIter != mSchedule.end();
         ++scheduleIter)
    {
        for (std::list<std::shared_ptr<Transmission>>::iterator iter = scheduleIter->begin();
            iter != scheduleIter->end();
            ++iter)
        {
            if ((*iter)->packet->frame.recipientAddr == recipientAddr)
            {
                ++n;
            }
        }
    }
    return n;
}

uint32_t PrioritizedTxScheduler::cancelAll()
{
    uint32_t n = 0;
    for (std::vector<std::list<std::shared_ptr<Transmission>>>::iterator scheduleIter = mSchedule.begin();
         scheduleIter != mSchedule.end();
         ++scheduleIter)
    {
        n += scheduleIter->size();
        scheduleIter->clear();
    }
    return n;
}
