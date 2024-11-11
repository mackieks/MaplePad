#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "hal/System/MutexInterface.hpp"
#include "pico/mutex.h"

// From raspberry-pi-pico-c-sdk.pdf:
// Mutex API for non IRQ mutual exclusion between cores.
// Unlike critical sections, the mutex protected code is not necessarily required/expected to
// complete quickly as no other sytem wide locks are held on account of an acquired mutex.
class Mutex : public MutexInterface
{
    public:
        Mutex();

        virtual ~Mutex();

        virtual void lock() final;

        virtual void unlock() final;

        virtual int8_t tryLock() final;

    private:
        mutex_t mMutex;
};

#endif // __MUTEX_H__
