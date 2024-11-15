#include "DreamcastMainPeripheral.hpp"

#include <assert.h>
#include <stdio.h>

namespace client
{

DreamcastMainPeripheral::DreamcastMainPeripheral(std::shared_ptr<MapleBusInterface> bus,
                                                 uint8_t addr,
                                                 uint8_t regionCode,
                                                 uint8_t connectionDirectionCode,
                                                 const char* descriptionStr,
                                                 const char* producerStr,
                                                 const char* versionStr,
                                                 float standbyCurrentmA,
                                                 float maxCurrentmA) :
    DreamcastPeripheral(addr,
                        regionCode,
                        connectionDirectionCode,
                        descriptionStr,
                        producerStr,
                        versionStr,
                        standbyCurrentmA,
                        maxCurrentmA),
    mBus(bus),
    mIsConnectionAllowed(true),
    mPlayerIndex(0),
    mSubPeripherals(),
    mLastSender(0),
    mPacketOut(),
    mLastPacketOut(),
    mPacketSent(false),
    mPacketIn(),
    mPlayerIndexChangedCb(nullptr),
    mReadCount(0)
{
    mPacketOut.reservePayload(256);
    mLastPacketOut.reservePayload(256);
    mPacketIn.reservePayload(256);
}

DreamcastMainPeripheral::DreamcastMainPeripheral(std::shared_ptr<MapleBusInterface> bus,
                                                 uint8_t addr,
                                                 uint8_t regionCode,
                                                 uint8_t connectionDirectionCode,
                                                 const char* descriptionStr,
                                                 const char* versionStr,
                                                 float standbyCurrentmA,
                                                 float maxCurrentmA) :
    DreamcastPeripheral(addr,
                        regionCode,
                        connectionDirectionCode,
                        descriptionStr,
                        versionStr,
                        standbyCurrentmA,
                        maxCurrentmA),
    mBus(bus),
    mIsConnectionAllowed(true),
    mPlayerIndex(-1),
    mSubPeripherals(),
    mLastSender(0),
    mPacketOut(),
    mLastPacketOut(),
    mPacketSent(false),
    mPacketIn(),
    mPlayerIndexChangedCb(nullptr),
    mReadCount(0)
{
    mPacketOut.reservePayload(256);
    mLastPacketOut.reservePayload(256);
    mPacketIn.reservePayload(256);
}

void DreamcastMainPeripheral::addSubPeripheral(std::shared_ptr<DreamcastPeripheral> subPeripheral)
{
    // This sub-peripheral's address must be unique
    assert(mSubPeripherals.find(subPeripheral->mAddr) == mSubPeripherals.end());
    assert(subPeripheral->mAddr != mAddr);
    // Add it
    mSubPeripherals.insert(std::make_pair(subPeripheral->mAddr, subPeripheral));
    // Accumulate to my address (main peripheral communicates back what sub peripherals are attached)
    mAddrAugmenter |= subPeripheral->mAddr;
}

bool DreamcastMainPeripheral::removeSubPeripheral(uint8_t addr)
{
    bool removed = false;
    std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>>::iterator iter = mSubPeripherals.find(addr);
    if (iter != mSubPeripherals.end())
    {
        mAddrAugmenter &= ~(iter->second->mAddr);
        mSubPeripherals.erase(iter);
        removed = true;
    }
    return removed;
}

bool DreamcastMainPeripheral::handlePacket(const MaplePacket& in, MaplePacket& out)
{
    uint8_t playerIdx = (in.frame.senderAddr & PLAYER_ID_ADDR_MASK) >> PLAYER_ID_BIT_SHIFT;
    setPlayerIndex(playerIdx);

    return DreamcastPeripheral::handlePacket(in, out);
}

bool DreamcastMainPeripheral::dispensePacket(const MaplePacket& in, MaplePacket& out)
{
    bool handled = false;
    bool valid = false;
    out.reset();

    uint8_t rawRecipientAddr = in.frame.recipientAddr & ~PLAYER_ID_ADDR_MASK;
    if (rawRecipientAddr == mAddr)
    {
        // This is for me
        ++mReadCount;
        if (mIsConnectionAllowed)
        {
            handled = true;
            valid = handlePacket(in, out);
        }
    }
    else if (mSubPeripherals.find(rawRecipientAddr) != mSubPeripherals.end())
    {
        // This is for one of my sub-peripherals
        if (mIsConnectionAllowed)
        {
            handled = true;
            valid = mSubPeripherals[rawRecipientAddr]->handlePacket(in, out);
        }
    }
    // else: not handled

    if (handled && !valid && out.frame.senderAddr != out.frame.recipientAddr)
    {
        out.frame.command = COMMAND_RESPONSE_UNKNOWN_COMMAND;
        out.payload.clear();
        out.updateFrameLength();
        valid = true;
    }

    return valid;
}

void DreamcastMainPeripheral::reset()
{
    DreamcastPeripheral::reset();
    mPlayerIndex = -1;
    for (std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>>::iterator iter = mSubPeripherals.begin();
         iter != mSubPeripherals.end();
         ++iter)
    {
        iter->second->reset();
    }

    if (mPlayerIndexChangedCb != nullptr)
    {
        mPlayerIndexChangedCb(-1);
    }
}

int16_t DreamcastMainPeripheral::getPlayerIndex()
{
    return mPlayerIndex;
}

void DreamcastMainPeripheral::setPlayerIndex(uint8_t idx)
{
    assert(idx < 4);
    if (!mConnected || idx != mPlayerIndex)
    {
        if (mConnected)
        {
            // About to make a connection - first reset
            reset();
        }
        // Set player index value in the augmenters
        mPlayerIndex = idx;
        uint8_t augmenterMask = (idx << PLAYER_ID_BIT_SHIFT);
        mAddrAugmenter = (mAddrAugmenter & ~PLAYER_ID_ADDR_MASK) | augmenterMask;
        for (std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>>::iterator iter = mSubPeripherals.begin();
             iter != mSubPeripherals.end();
             ++iter)
        {
            // The only augmenter in sub-peripherals is player index
            iter->second->setAddrAugmenter(augmenterMask);
        }

        if (mPlayerIndexChangedCb != nullptr)
        {
            mPlayerIndexChangedCb(mPlayerIndex);
        }
    }
}

void DreamcastMainPeripheral::task(uint64_t currentTimeUs)
{
    MapleBusInterface::Status status = mBus->processEvents(currentTimeUs);

    switch (status.phase)
    {
        case MapleBusInterface::Phase::READ_FAILED:
        {
            if (status.failureReason == MapleBusInterface::FailureReason::CRC_INVALID
                && isConnected())
            {
                // Data doesn't look right - tell host to resend
                mPacketOut.reset();
                mPacketOut.frame.command = COMMAND_RESPONSE_REQUEST_RESEND;
                mPacketOut.frame.recipientAddr = mLastSender;
                mPacketOut.frame.senderAddr = getAddress();
                mPacketOut.updateFrameLength();
                (void)mBus->write(mPacketOut, true, READ_TIMEOUT_US);
            }
            else
            {
                // Nothing received for a while; reset peripheral
                reset();
                // Start waiting for directive from host
                (void)mBus->startRead(READ_TIMEOUT_US);
            }
        }
        break;

        case MapleBusInterface::Phase::INVALID: // Fall through
        case MapleBusInterface::Phase::IDLE: // Fall through
        default:
        {
            // Initialization or invalid state
            reset();
        }
        // Fall through
        case MapleBusInterface::Phase::WRITE_FAILED: // Fall through
        case MapleBusInterface::Phase::WRITE_COMPLETE:
        {
            // Start waiting for directive from host
            (void)mBus->startRead(READ_TIMEOUT_US);
        }
        break;

        case MapleBusInterface::Phase::WRITE_IN_PROGRESS: // Fall through
        case MapleBusInterface::Phase::WAITING_FOR_READ_START: // Fall through
        case MapleBusInterface::Phase::READ_IN_PROGRESS:
        {
            // Nothing to do (waiting for current process to complete)
        }
        break;

        case MapleBusInterface::Phase::READ_COMPLETE:
        {
            bool writeIt = false;

            mPacketIn.set(status.readBuffer, status.readBufferLen);
            mLastSender = mPacketIn.frame.senderAddr;

            if (mPacketIn.frame.command == COMMAND_RESPONSE_REQUEST_RESEND)
            {
                if (mPacketSent)
                {
                    // Write the previous packet
                    mPacketOut = mLastPacketOut;
                    writeIt = true;
                }
            }
            else
            {
                //dispensePacket() calls the peripherals handlePacket()
                writeIt = dispensePacket(mPacketIn, mPacketOut);
            }

            if (writeIt)
            {
                mPacketSent = true;
                mLastPacketOut = mPacketOut;
                (void)mBus->write(mPacketOut, true, READ_TIMEOUT_US);
            }
            else
            {
                // Write nothing, and the host will eventually stall out
                (void)mBus->startRead(READ_TIMEOUT_US);
            }
        }
        break;
    }
}

void DreamcastMainPeripheral::setPlayerIndexChangedCb(PlayerIndexChangedFn fn)
{
    mPlayerIndexChangedCb = fn;
}

}
