#pragma once

#include <stdint.h>

class SystemMemory
{
public:
    //! Virtual destructor
    virtual ~SystemMemory() {}

    //! @returns number of bytes reserved in memory
    virtual uint32_t getMemorySize() = 0;

    //! Reads from memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in,out] size  Number of bytes to read, set with the number of bytes read
    //! @returns a pointer containing the number of bytes returned in size
    virtual const uint8_t* read(uint32_t offset, uint32_t& size) = 0;

    //! Writes to memory - must return within 500 microseconds
    //! @param[in] offset  Offset into memory in bytes
    //! @param[in] data  The data to write
    //! @param[in,out] size  Number of bytes to write, set with number of bytes written
    //! @returns true iff all bytes were written or at least queued for write
    virtual bool write(uint32_t offset, const void* data, uint32_t& size) = 0;

    //! Used to determine read/write status for status LED
    //! @returns the time of last read/write activity
    virtual uint64_t getLastActivityTime() = 0;
};
