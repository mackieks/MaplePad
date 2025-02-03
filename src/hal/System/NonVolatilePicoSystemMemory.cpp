#include "NonVolatilePicoSystemMemory.hpp"

#include "hal/System/LockGuard.hpp"

#include "hardware/flash.h"
#include "pico/stdlib.h"

#include <assert.h>
#include <string.h>
#include <algorithm>


NonVolatilePicoSystemMemory::NonVolatilePicoSystemMemory(uint32_t flashOffset, uint32_t size) :
    SystemMemory(),
    mOffset(flashOffset),
    mSize(size),
    mLocalMem(size),
    mMutex(),
    mProgrammingState(ProgrammingState::WAITING_FOR_JOB),
    mSectorQueue(),
    mDelayedWriteTime(0),
    mLastActivityTime(0),
    mCurrentPage(1)
{
    assert(flashOffset % SECTOR_SIZE == 0);

    // Copy all of flash into volatile memory
    const uint8_t* const readFlash = (const uint8_t *)(XIP_BASE + mOffset);
    mLocalMem.write(0, readFlash, size);
}

void NonVolatilePicoSystemMemory::nextPage(uint32_t size)
{
    ++mCurrentPage;

    // Once max pages reached, cycled back to beginning
    if(mCurrentPage > MAX_NUM_PAGES)
    {
        mCurrentPage = MIN_NUM_PAGES;
    }

    setPageBlock(size, mCurrentPage);
}

void NonVolatilePicoSystemMemory::prevPage(uint32_t size)
{
    --mCurrentPage;

    // Once max pages reached, cycled back to beginning
    if(mCurrentPage < MIN_NUM_PAGES)
    {
        mCurrentPage = MAX_NUM_PAGES;
    }

    setPageBlock(size, mCurrentPage);
}

void NonVolatilePicoSystemMemory::setPageBlock(uint32_t size, uint8_t page)
{
    uint32_t flashOffset = PICO_FLASH_SIZE_BYTES - (size * page);
    // Assert that the flash offset is aligned with the sector size
    assert((flashOffset) % SECTOR_SIZE == 0);

    // Initialize members
    mOffset = flashOffset;
    mSize = size;
    mProgrammingState = ProgrammingState::WAITING_FOR_JOB;
    mSectorQueue.clear(); // Empty queue by default
    mDelayedWriteTime = 0;
    mLastActivityTime = 0;

    // Copy all of flash into volatile memory
    const uint8_t* const readFlash = (const uint8_t *)(XIP_BASE + mOffset);
    mLocalMem.write(0, readFlash, size);

    notify(page);

    sleep_ms(250); // Settle
}

uint32_t NonVolatilePicoSystemMemory::getMemorySize()
{
    return mSize;
}

const uint8_t* NonVolatilePicoSystemMemory::read(uint32_t offset, uint32_t& size)
{
    mLastActivityTime = time_us_64();
    // A copy of memory is kept in RAM because nothing can be read from flash while erase is
    // processing which takes way too long for this to return within 500 microseconds
    return mLocalMem.read(offset, size);
}

bool NonVolatilePicoSystemMemory::write(uint32_t offset, const void* data, uint32_t& size)
{
    // This entire function is serialized with process()
    LockGuard lock(mMutex, true);

    mLastActivityTime = time_us_64();

    // First, store this data into local RAM
    bool success = mLocalMem.write(offset, data, size);

    if (size > 0)
    {
        uint16_t firstSector = offset / SECTOR_SIZE;
        uint16_t lastSector = (offset + size - 1) / SECTOR_SIZE;
        bool delayWrite = false;
        bool itemAdded = false;
        for (uint32_t i = firstSector; i <= lastSector; ++i)
        {
            std::list<uint16_t>::iterator it =
                std::find(mSectorQueue.begin(), mSectorQueue.end(), i);

            if (it == mSectorQueue.end())
            {
                // Add this sector
                mSectorQueue.push_back(i);
                itemAdded = true;
            }
            else if (it == mSectorQueue.begin())
            {
                // Currently processing this sector - delay write even further
                delayWrite = true;
            }
        }

        if (itemAdded)
        {
            // No longer need to delay write because caller moved on to another sector
            mDelayedWriteTime = 0;
        }
        else if (delayWrite)
        {
            setWriteDelay();
        }
    }

    return success;
}

uint64_t NonVolatilePicoSystemMemory::getLastActivityTime()
{
    // WARNING: Not an atomic read, but this isn't a critical thing anyway
    return mLastActivityTime;
}

void NonVolatilePicoSystemMemory::process()
{
    mMutex.lock();
    bool unlock = true;

    switch (mProgrammingState)
    {
        case ProgrammingState::WAITING_FOR_JOB:
        {
            if (!mSectorQueue.empty())
            {
                uint16_t sector = *mSectorQueue.begin();
                uint32_t flashByte = sectorToFlashByte(sector);

                mLastActivityTime = time_us_64();
                setWriteDelay();
                mProgrammingState = ProgrammingState::SECTOR_ERASING;

                // flash_range_erase blocks until erase is complete, so don't hold the lock
                // TODO: It should be possible to execute a non-blocking erase command then
                //       periodically check status within SECTOR_ERASING state until complete.
                //       It's not that important at the moment because this is the only process
                //       running in core 1.
                mMutex.unlock();
                unlock = false;
                flash_range_erase(flashByte, SECTOR_SIZE);
            }
        }
        break;

        case ProgrammingState::SECTOR_ERASING:
        {
            // Nothing to do in this state
            mProgrammingState = ProgrammingState::DELAYING_WRITE;
        }
        // Fall through

        case ProgrammingState::DELAYING_WRITE:
        {
            mLastActivityTime = time_us_64();
            // Write is delayed until the host moves on to writing another sector or if timeout
            // is reached. This helps ensure that the same sector isn't written multiple times.
            if (time_us_64() >= mDelayedWriteTime)
            {
                uint16_t sector = *mSectorQueue.begin();
                uint32_t localByte = sector * SECTOR_SIZE;
                uint32_t size = SECTOR_SIZE;
                const uint8_t* mem = mLocalMem.read(localByte, size);

                assert(mem != nullptr);

                uint32_t flashByte = sectorToFlashByte(sector);
                flash_range_program(flashByte, mem, SECTOR_SIZE);

                mSectorQueue.pop_front();
                mProgrammingState = ProgrammingState::WAITING_FOR_JOB;
            }
        }
        break;

        default:
        {
            assert(false);
        }
        break;
    }

    if (unlock)
    {
        mMutex.unlock();
    }
}

uint32_t NonVolatilePicoSystemMemory::sectorToFlashByte(uint16_t sector)
{
    return mOffset + (sector * SECTOR_SIZE);
}

void NonVolatilePicoSystemMemory::setWriteDelay()
{
    mDelayedWriteTime = time_us_64() + WRITE_DELAY_US;
}

std::array<uint8_t, 64> NonVolatilePicoSystemMemory::fetchSettingsFromFlash()
{
    std::array<uint8_t, 64> settingsData = {0};
    const uint8_t* const readFlash = (const uint8_t *)(XIP_BASE + mOffset);
    memcpy(settingsData.data(), readFlash, mSize);

    return settingsData;
}

void NonVolatilePicoSystemMemory::writeSettingsToFlash(std::array<uint8_t, 64> settingsData)
{
    uint interrupts = save_and_disable_interrupts();
    flash_range_erase(mOffset, FLASH_SECTOR_SIZE);
    flash_range_program(mOffset, settingsData.data(), FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);
}

/*
        xMin = 0x00;
        xCenter = 0x80;
        xMax = 0xff;
        xDeadzone = 0x0f;
        xAntiDeadzone = 0x04;
        invertX = 0;

        yMin = 0x00;
        yCenter = 0x00;
        yMax = 0xff;
        yDeadzone = 0x0f;
        yAntiDeadzone = 0x04;
        invertY = 0;

        lMin = 0x00;
        lMax = 0xff;
        lDeadzone = 0x00;
        lAntiDeadzone = 0x04;
        invertL = 0;

        rMin = 0x00;
        rMax = 0xff;
        rDeadzone = 0x00;
        rAntiDeadzone = 0x04;
        invertR = 0;

        oledFlip = 0;
        swapXY = 0;
        swapLR = 0;
        autoResetEnable = 0;
        autoResetTimer = 0x5A; // 180s
        version = CURRENT_FW_VERSION;
*/
std::array<uint8_t, 64> NonVolatilePicoSystemMemory::getDefaultSettings()
{
    std::array<uint8_t, 64> data = {0};

    data[0] = 0x80;
    data[1] = 0x00;
    data[2] = 0xff;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0xff;
    data[6] = 0x00;
    data[7] = 0xff;
    data[8] = 0x00;
    data[9] = 0xff;
    data[10] = 0;
    data[11] = 0;
    data[12] = 0;
    data[13] = 0;
    data[14] = 1;
    data[15] = 1;
    data[16] = 0;
    data[17] = 1;
    data[18] = 0;
    data[19] = 0;
    data[20] = 0;
    data[23] = 0x0f;
    data[24] = 0x04;
    data[25] = 0x0f;
    data[26] = 0x04;
    data[27] = 0x00;
    data[28] = 0x04;
    data[29] = 0x00;
    data[30] = 0x04;
    data[31] = 0;
    data[32] = 0x5A;
    data[33] = FW_MAJOR_VERSION;
    data[34] = FW_MINOR_VERSION;

    return data;
}