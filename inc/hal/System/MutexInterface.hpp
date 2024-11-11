#ifndef __MUTEX_INTERFACE_H__
#define __MUTEX_INTERFACE_H__

#include <stdint.h>

//! This interface is used to decouple mutex functionality in HAL from the Dreamcast functionality.
class MutexInterface
{
    public:
        //! Constructor
        MutexInterface() {}

        //! Virtual destructor
        virtual ~MutexInterface() {}

        //! Blocks until mutex is available and then takes it
        virtual void lock() = 0;

        //! Releases the previously locked mutex
        virtual void unlock() = 0;

        //! Tries to obtain mutex without blocking
        //! @returns 1 if mutex was obtained
        //! @returns 0 if mutex was not obtained, and blocking would be valid in this context
        //! @returns -1 if mutex was not obtained, and blocking would cause deadlock
        virtual int8_t tryLock() = 0;
};

#endif // __MUTEX_INTERFACE_H__
