#pragma once

#include "hal/System/SystemMemory.hpp"
#include "hal/Display/TransientOverlaySubject.hpp"
#include "VolatileSystemMemory.hpp"
#include "Mutex.hpp"

#include <memory>
#include <list>

// The Raspberry Pi Pico uses an external flash chip, the W25Q16JV, to store code. There are some
// limitations which make it difficult to use as storage space.
// Datasheet: https://www.winbond.com/resource-files/w25q16jv%20spi%20revh%2004082019%20plus.pdf
// For W25Q16JV:
// - Read or write of 256 byte page will take approximately 4 microseconds
// - Write can only keep a bit "1" or flip a bit from "1" to "0" (true for most flash)
// - Erasing sets all bits to "1" within a 4096 byte sector which takes up to 400 ms
// - The XIP (execute in place) feature cannot be used during write/erasure

// The Pico uses the XIP feature of this chip to execute code. This means it is necessary to run
// code from RAM while accessing the flash in code. The time that it takes to erase memory also puts
// a damper on things. The only way that I could think to make this work is to enable copy_to_ram
// for the executable so that the entire program loads into RAM before executing. Then the second
// core can be used to do erase and write in the background. Because nothing can be read from flash
// while erase is executing, a copy of memory is also stored in RAM so that read() returns quickly.
// Ultimately, this limits program size up to 128 KB (assuming 128 KB in RAM is used for
// the local copy of non-volatile memory). That size of course becomes more limited the more RAM is
// used for the program itself.

//! SystemMemory class using Pico's onboard flash
//! In order for this to work properly, entire program must be running from RAM. No other component
//! within the program may access flash when this class is used. One core may call read() and
//! write() while the other core must call process() to process queued writes. It should be possible
//! to do all execution from a single core in the future once the TODO within process() is
//! addressed.
class NonVolatilePicoSystemMemory : public SystemMemory
{
public:
    //! Flash memory write states
    enum class ProgrammingState
    {
        //! Waiting for write()
        WAITING_FOR_JOB = 0,
        //! Sector erase sent, waiting for erase to complete
        SECTOR_ERASING,
        //! Delaying write to erased sector in case more writes come in for same sector
        DELAYING_WRITE,
    };

    //! Constructor
    //! @param[in] flashOffset  Offset into flash, must align to SECTOR_SIZE
    //! @param[in] size  Number of bytes to allow read/write
    NonVolatilePicoSystemMemory(uint32_t flashOffset, uint32_t size);

    //! @returns number of bytes reserved in memory
    virtual uint32_t getMemorySize() final;

    //! Reads from memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in,out] size  Number of bytes to read, set with the number of bytes read
    //! @returns a pointer containing the number of bytes returned in size
    virtual const uint8_t* read(uint32_t offset, uint32_t& size) final;

    //! Writes to memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in] data  The data to write
    //! @param[in,out] size  Number of bytes to write, set with number of bytes written
    //! @returns true iff all bytes were written or at least queued for write
    virtual bool write(uint32_t offset, const void* data, uint32_t& size) final;

    //! Used to determine read/write status for status LED
    //! @returns the time of last read/write activity
    virtual uint64_t getLastActivityTime() final;

    //! Must be called to periodically process flash access
    //! @warning this may block for up to 400 ms
    void process();

    //! Go to the next available VMU page. Cycles back to the first page once the end is reached.
    //! @returns the page number of the loaded block
    void nextPage(uint32_t size);

    //! Go to the previous available VMU page. Cycles back to the last page once the first page is reached.
    //! @returns the page number of the loaded block
    void prevPage(uint32_t ssize_t);

private:
    //! Converts a local sector index to flash byte offset
    uint32_t sectorToFlashByte(uint16_t sector);

    //! Set the write delay using the current time
    void setWriteDelay();

    void setPageBlock(uint32_t size, uint8_t page);

private:
    //! Number of bytes in a sector
    static const uint32_t SECTOR_SIZE = 4096;
    //! Page size in bytes
    static const uint32_t PAGE_SIZE = 256;
    //! How long to delay before committing to the last sector write
    static const uint32_t WRITE_DELAY_US = 200000;
    //! Flash memory offset
    uint32_t mOffset;
    //! Number of bytes in volatile memory
    uint32_t mSize;
    //! Because erase takes so long which prevents read, the entire flash range is copied locally
    VolatileSystemMemory mLocalMem;
    //! Mutex to serialize write() and flash programming
    Mutex mMutex;
    //! Keeps track of the state in process()
    ProgrammingState mProgrammingState;
    //! Queue of sectors which needs to be written to flash
    std::list<uint16_t> mSectorQueue;
    //! The time at which write should execute after erase has completed
    //! (or 0 to execute on next process())
    uint64_t mDelayedWriteTime;
    //! Last system time of read/write activity
    uint64_t mLastActivityTime;

    uint8_t mCurrentPage;

    std::list<std::shared_ptr<TransientOverlayObserver>> observerList;

    static const uint8_t MAX_NUM_PAGES = 8;
    static const uint8_t MIN_NUM_PAGES = 1;
};
