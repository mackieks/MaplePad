#ifndef __USB_FILE_SYSTEM_H__
#define __USB_FILE_SYSTEM_H__

#include <stdint.h>

class UsbFile;

//! Interface for USB MSC to receive files
class UsbFileSystem
{
    public:
        //! Virtual destructor
        virtual ~UsbFileSystem() {}
        //! Add a file to the mass storage device
        virtual void add(UsbFile* file) = 0;
        //! Remove a file from the mass storage device
        virtual void remove(UsbFile* file) = 0;
};

#endif // __USB_FILE_SYSTEM_H__
