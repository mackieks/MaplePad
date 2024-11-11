#ifndef __USB_FILE_H__
#define __USB_FILE_H__

#include <stdint.h>

//! Interface for a dreamcast device to inherit when it must show as a file on mass storage
class UsbFile
{
    public:
        //! Virtual destructor
        virtual ~UsbFile() {}
        //! @returns file name
        virtual const char* getFileName() = 0;
        //! @returns file size in bytes (currently only up to 128KB supported)
        virtual uint32_t getFileSize() = 0;
        //! @returns true iff this file is read only
        virtual bool isReadOnly() = 0;
        //! Blocking read (must only be called from the core not operating maple bus)
        //! @param[in] blockNum  Block number to read (a block is 512 bytes)
        //! @param[out] buffer  Buffer output
        //! @param[in] bufferLen  The length of buffer (only up to 512 bytes will be read)
        //! @param[in] timeoutUs  Timeout in microseconds
        //! @returns Positive value indicating how many bytes were read
        //! @returns Zero if read failure occurred
        //! @returns Negative value if timeout elapsed
        virtual int32_t read(uint8_t blockNum,
                             void* buffer,
                             uint16_t bufferLen,
                             uint32_t timeoutUs) = 0;
        //! Blocking write (must only be called from the core not operating maple bus)
        //! @param[in] blockNum  Block number to write (block is 512 bytes)
        //! @param[in] buffer  Buffer
        //! @param[in] bufferLen  The length of buffer (but only up to 512 bytes will be written)
        //! @param[in] timeoutUs  Timeout in microseconds
        //! @returns Positive value indicating how many bytes were written
        //! @returns Zero if write failure occurred
        //! @returns Negative value if timeout elapsed
        virtual int32_t write(uint8_t blockNum,
                              const void* buffer,
                              uint16_t bufferLen,
                              uint32_t timeoutUs) = 0;
};

#endif // __USB_FILE_H__
