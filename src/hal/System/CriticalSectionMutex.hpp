#ifndef __CRITICAL_SECTION_MUTEX_H__
#define __CRITICAL_SECTION_MUTEX_H__

#include "hal/System/MutexInterface.hpp"
#include "pico/critical_section.h"

// From raspberry-pi-pico-c-sdk.pdf:
// Critical Section API for short-lived mutual exclusion safe for IRQ and multi-core.
// A critical section is non-reentrant, and provides mutual exclusion using a spin-lock to prevent
// access from the other core, and from (higher priority) interrupts on the same core. It does the
// former using a spin lock and the latter by disabling interrupts on the calling core. Because
// interrupts are disabled when a critical_section is owned, uses of the critical_section should be
// as short as possible.
class CriticalSectionMutex : public MutexInterface
{
    public:
        CriticalSectionMutex();

        virtual ~CriticalSectionMutex();

        virtual void lock() final;

        virtual void unlock() final;

        virtual int8_t tryLock() final;

    private:
        critical_section_t mCriticalSection;
};

#endif // __CRITICAL_SECTION_MUTEX_H__
