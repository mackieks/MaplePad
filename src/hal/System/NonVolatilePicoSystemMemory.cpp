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

uint8_t* NonVolatilePicoSystemMemory::fetchSettingsFromFlash()
{
    uint8_t* settingsData = new uint8_t[64];
    const uint8_t* const readFlash = (const uint8_t *)(XIP_BASE + mOffset);
    memcpy(settingsData, readFlash, mSize);

    return settingsData;
}

void NonVolatilePicoSystemMemory::writeSettingsToFlash(uint8_t* data)
{
    uint interrupts = save_and_disable_interrupts();
    flash_range_erase(mOffset, FLASH_SECTOR_SIZE);
    flash_range_program(mOffset, data, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);
}