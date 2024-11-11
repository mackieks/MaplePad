#include "DreamcastStorage.hpp"
#include "dreamcast_constants.h"
#include "utils.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

DreamcastStorage::DreamcastStorage(uint8_t addr,
                                   uint32_t fd,
                                   std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                   PlayerData playerData) :
    DreamcastPeripheral("storage", addr, fd, scheduler, playerData.playerIndex),
    mExiting(false),
    mClock(playerData.clock),
    mUsbFileSystem(playerData.fileSystem),
    mFileName{},
    mReadState(READ_WRITE_IDLE),
    mReadingTxId(0),
    mReadingBlock(-1),
    mReadPacket(nullptr),
    mReadKillTime(0),
    mWriteState(READ_WRITE_IDLE),
    mWritingTxId(0),
    mWritingBlock(0),
    mWriteBuffer(nullptr),
    mWriteBufferLen(0),
    mWriteKillTime(0),
    mWritePhase(0),
    mMinDurationBetweenWrites(DEFAULT_MIN_DURATION_US_BETWEEN_WRITES),
    mLastWriteTimeUs(0)
{
    // Memory access functionality relies on:
    // - 512 byte blocks (for UsbFile)
    // - No CRC (I'm not bothering with this computation)
    // - WA count equally divides words (I'm not bothering with partial splits)
    // - RA count is 1 (I'm not bothering with it otherwise)
    // Don't add the file if either of these are not true
    if (getBytesPerBlock() == 512
        && !isCrcRequired()
        && (((512 / sizeof(uint32_t)) % getWriteAccesCount())) == 0
        && getReadAccessCount() == 1)
    {
        int32_t idx = subPeripheralIndex(mAddr);
        if (idx >= 0)
        {
            if (idx == 0)
            {
                snprintf(mFileName, sizeof(mFileName), "vmu%lu.bin", (long unsigned int)mPlayerIndex);
            }
            else
            {
                snprintf(mFileName, sizeof(mFileName), "vmu%lu-%li.bin", (long unsigned int)mPlayerIndex, (long int)idx);
            }

            mUsbFileSystem.add(this);
        }
    }
}

DreamcastStorage::~DreamcastStorage()
{
    mExiting = true;
    // The following is externally serialized with any read() call
    mUsbFileSystem.remove(this);
}

void DreamcastStorage::task(uint64_t currentTimeUs)
{
    switch(mReadState)
    {
        case READ_WRITE_STARTED:
        {
            uint32_t payload[2] = {FUNCTION_CODE, mReadingBlock};
            mReadingTxId = mEndpointTxScheduler->add(
                PrioritizedTxScheduler::TX_TIME_ASAP,
                this,
                COMMAND_BLOCK_READ,
                payload,
                2,
                true,
                130);
            mReadState = READ_WRITE_SENT;
        }
        break;

        case READ_WRITE_SENT:
        {
            if (currentTimeUs >= mReadKillTime)
            {
                // Timeout
                mEndpointTxScheduler->cancelById(mReadingTxId);
                mReadState = READ_WRITE_IDLE;
            }
        }
        break;

        case READ_WRITE_PROCESSING:
            // Already processing, so no need to check timeout value
            // FALL THROUGH
        default:
            break;
    }

    switch(mWriteState)
    {
        case READ_WRITE_STARTED:
        {
            mWritePhase = 0;
            mMinDurationBetweenWrites = DEFAULT_MIN_DURATION_US_BETWEEN_WRITES;
            // Build the payload with write data
            queueNextWritePhase();
        }
        break;

        case READ_WRITE_SENT:
        {
            if (currentTimeUs >= mWriteKillTime)
            {
                // Timeout
                mEndpointTxScheduler->cancelById(mWritingTxId);
                mWritePhase = getWriteAccesCount();
                queueWriteCommit();
            }
        }
        break;

        case READ_WRITE_PROCESSING:
            // Already processing, so no need to check timeout value
            // FALL THROUGH
        case WRITE_COMMIT_SENT:
            // Don't interrupt commit process
            // FALL THROUGH
        default:
            break;
    }
}

void DreamcastStorage::txStarted(std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_WRITE_IDLE && tx->transmissionId == mReadingTxId)
    {
        mReadState = READ_WRITE_PROCESSING;
    }
    if (mWriteState != READ_WRITE_IDLE && tx->transmissionId == mWritingTxId)
    {
        mWriteState = READ_WRITE_PROCESSING;
    }
}

void DreamcastStorage::txFailed(bool writeFailed,
                                bool readFailed,
                                std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_WRITE_IDLE && tx->transmissionId == mReadingTxId)
    {
        // Failure
        mReadState = READ_WRITE_IDLE;
    }
    if (mWriteState != READ_WRITE_IDLE && tx->transmissionId == mWritingTxId)
    {
        // Failure
        mLastWriteTimeUs = mClock.getTimeUs();
        mMinDurationBetweenWrites += DURATION_US_BETWEEN_WRITES_INC;
        if (mLastWriteTimeUs + getWriteAccesCount() * mMinDurationBetweenWrites > mWriteKillTime)
        {
            mWriteBufferLen = -1;
            mWriteState = READ_WRITE_IDLE;
        }
        else
        {
            // Try again
            mWritePhase = 0;
            queueNextWritePhase();
        }
    }
}

void DreamcastStorage::txComplete(std::shared_ptr<const MaplePacket> packet,
                                  std::shared_ptr<const Transmission> tx)
{
    if (mReadState != READ_WRITE_IDLE && tx->transmissionId == mReadingTxId)
    {
        // Complete!
        mReadPacket = packet;
        mReadState = READ_WRITE_IDLE;
    }
    if (mWriteState != READ_WRITE_IDLE && tx->transmissionId == mWritingTxId)
    {
        mLastWriteTimeUs = mClock.getTimeUs();
        if (packet->frame.command == COMMAND_RESPONSE_ACK)
        {
            if (++mWritePhase >= getWriteAccesCount())
            {
                if (tx->packet->frame.command == COMMAND_GET_LAST_ERROR)
                {
                    // Complete!
                    mWriteState = READ_WRITE_IDLE;
                }
                else
                {
                    // Queue the message which commits the written data
                    queueWriteCommit();
                }
            }
            else
            {
                // Queue up next phase
                queueNextWritePhase();
            }
        }
        else
        {
            mMinDurationBetweenWrites += DURATION_US_BETWEEN_WRITES_INC;
            if (mLastWriteTimeUs + getWriteAccesCount() * mMinDurationBetweenWrites > mWriteKillTime)
            {
                mWriteBufferLen = -1;
                mWriteState = READ_WRITE_IDLE;
            }
            else
            {
                // Try again
                mWritePhase = 0;
                queueNextWritePhase();
            }
        }
    }
}

void DreamcastStorage::queueNextWritePhase()
{
    uint32_t numBlockWords = mWriteBufferLen / 4 / getWriteAccesCount();
    uint32_t numPayloadWords = 2 + numBlockWords;
    uint32_t payload[numPayloadWords] = {FUNCTION_CODE, mWritingBlock | ((uint32_t)mWritePhase << 16)};
    const uint32_t* pDataIn = static_cast<const uint32_t*>(mWriteBuffer);
    pDataIn += (mWritePhase * numBlockWords);
    uint32_t *pDataOut = &payload[2];
    for (uint32_t i = 0; i < numBlockWords; ++i, ++pDataIn, ++pDataOut)
    {
        *pDataOut = flipWordBytes(*pDataIn);
    }

    mWritingTxId = mEndpointTxScheduler->add(
        mLastWriteTimeUs + mMinDurationBetweenWrites,
        this,
        COMMAND_BLOCK_WRITE,
        payload,
        numPayloadWords,
        true,
        0);

    mWriteState = READ_WRITE_SENT;
}

void DreamcastStorage::queueWriteCommit()
{
    // Send COMMAND_GET_LAST_ERROR which commits written data
    uint32_t numPayloadWords = 2;
    uint32_t payload[numPayloadWords] = {FUNCTION_CODE, mWritingBlock | ((uint32_t)mWritePhase << 16)};

    mWritingTxId = mEndpointTxScheduler->add(
        mLastWriteTimeUs + mMinDurationBetweenWrites,
        this,
        COMMAND_GET_LAST_ERROR,
        payload,
        numPayloadWords,
        true,
        0);

    mWriteState = WRITE_COMMIT_SENT;
}

const char* DreamcastStorage::getFileName()
{
    return mFileName;
}

uint32_t DreamcastStorage::getFileSize()
{
    return (128 * 1024);
}

bool DreamcastStorage::isReadOnly()
{
    return (getWriteAccesCount() == 0);
}

int32_t DreamcastStorage::read(uint8_t blockNum,
                               void* buffer,
                               uint16_t bufferLen,
                               uint32_t timeoutUs)
{
    assert(mReadState == READ_WRITE_IDLE);
    // Set data
    mReadingTxId = 0;
    mReadingBlock = blockNum;
    mReadPacket = nullptr;
    mReadKillTime = mClock.getTimeUs() + timeoutUs;
    // Commit it
    mReadState = READ_WRITE_STARTED;

    // Wait for maple bus state machine to finish read
    // I'm not too happy about this blocking operation, but it works
    while(mReadState != READ_WRITE_IDLE && !mExiting);

    int32_t numRead = -1;
    if (mReadPacket)
    {
        uint16_t copyLen = (bufferLen > (mReadPacket->payload.size() * 4)) ? (mReadPacket->payload.size() * 4) : bufferLen;
        // Need to flip each word before copying
        uint8_t* buffer8 = (uint8_t*)buffer;
        for (uint32_t i = 2; i < (2U + (bufferLen / 4)); ++i)
        {
            uint32_t flippedWord = flipWordBytes(mReadPacket->payload[i]);
            memcpy(buffer8, &flippedWord, 4);
            buffer8 += 4;
        }
        numRead = copyLen;
    }

    mReadPacket = nullptr;

    return numRead;
}

int32_t DreamcastStorage::write(uint8_t blockNum,
                                const void* buffer,
                                uint16_t bufferLen,
                                uint32_t timeoutUs)
{
    if (!isReadOnly())
    {
        assert(mWriteState == READ_WRITE_IDLE);
        assert(bufferLen % 4 == 0);
        // Set data
        mWritingBlock = blockNum;
        mWriteBuffer = buffer;
        mWriteBufferLen = bufferLen;
        mWritingTxId = 0;
        mWriteKillTime = mClock.getTimeUs() + timeoutUs;
        // Commit it
        mWriteState = READ_WRITE_STARTED;

        // Wait for maple bus state machine to finish read
        // I'm not too happy about this blocking operation, but it works
        while(mWriteState != READ_WRITE_IDLE && !mExiting);

        return mWriteBufferLen;
    }
    return -1;
}

uint32_t DreamcastStorage::flipWordBytes(const uint32_t& word)
{
    return (word << 24) | (word << 8 & 0xFF0000) | (word >> 8 & 0xFF00) | (word >> 24);
}
