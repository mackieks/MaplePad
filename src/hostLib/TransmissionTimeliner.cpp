#include "TransmissionTimeliner.hpp"
#include <assert.h>

TransmissionTimeliner::TransmissionTimeliner(MapleBusInterface& bus, std::shared_ptr<PrioritizedTxScheduler> schedule):
    mBus(bus), mSchedule(schedule), mCurrentTx(nullptr)
{}

TransmissionTimeliner::ReadStatus TransmissionTimeliner::readTask(uint64_t currentTimeUs)
{
    ReadStatus status;

    // Process bus events and get any data received
    MapleBusInterface::Status busStatus = mBus.processEvents(currentTimeUs);
    status.busPhase = busStatus.phase;
    if (status.busPhase == MapleBusInterface::Phase::READ_COMPLETE)
    {
        status.received = std::make_shared<MaplePacket>(busStatus.readBuffer,
                                                        busStatus.readBufferLen);
        status.transmission = mCurrentTx;
        mCurrentTx = nullptr;
    }
    else if (status.busPhase == MapleBusInterface::Phase::WRITE_COMPLETE
             || status.busPhase == MapleBusInterface::Phase::READ_FAILED
             || status.busPhase == MapleBusInterface::Phase::WRITE_FAILED)
    {
        status.transmission = mCurrentTx;
        mCurrentTx = nullptr;
    }

    return status;
}

std::shared_ptr<const Transmission> TransmissionTimeliner::writeTask(uint64_t currentTimeUs)
{
    std::shared_ptr<const Transmission> txSent = nullptr;

    if (!mBus.isBusy())
    {
        PrioritizedTxScheduler::ScheduleItem item = mSchedule->peekNext(currentTimeUs);
        txSent = item.getTx();
        if (txSent != nullptr)
        {
            if (mBus.write(*txSent->packet, txSent->expectResponse))
            {
                mCurrentTx = txSent;
                mSchedule->popItem(item);
            }
            else
            {
                txSent = nullptr;
            }
        }
    }

    return txSent;
}