#include "CriticalSectionMutex.hpp"

CriticalSectionMutex::CriticalSectionMutex()
{
    critical_section_init(&mCriticalSection);
}

CriticalSectionMutex::~CriticalSectionMutex() {}

void CriticalSectionMutex::lock()
{
    critical_section_enter_blocking(&mCriticalSection);
}

void CriticalSectionMutex::unlock()
{
    critical_section_exit(&mCriticalSection);
}

int8_t CriticalSectionMutex::tryLock()
{
    // "try" isn't valid for critical section mutex, but taking will never cause deadlock since
    // interrupts are disabled as part of the locking mechanism.
    return 0;
}