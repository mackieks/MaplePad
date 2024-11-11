#include "Mutex.hpp"

Mutex::Mutex()
{
    mutex_init(&mMutex);
}

Mutex::~Mutex() {}

void Mutex::lock()
{
    mutex_enter_blocking(&mMutex);
}

void Mutex::unlock()
{
    mutex_exit(&mMutex);
}

int8_t Mutex::tryLock()
{
    uint32_t owner;
    if (!mutex_try_enter(&mMutex, &owner))
    {
        if (owner == get_core_num())
        {
            return -1; // would deadlock
        }
        else
        {
            return 0;
        }
    }
    return 1;
}
