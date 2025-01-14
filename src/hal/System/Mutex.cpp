#include "Mutex.hpp"

Mutex::Mutex()
{
    mutex_init(&mMutex);
}

Mutex::~Mutex() {}

void __no_inline_not_in_flash_func(Mutex::lock)(void)
{
    mutex_enter_blocking(&mMutex);
}

void __no_inline_not_in_flash_func(Mutex::unlock)(void)
{
    mutex_exit(&mMutex);
}

int8_t __no_inline_not_in_flash_func(Mutex::tryLock)(void)
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
