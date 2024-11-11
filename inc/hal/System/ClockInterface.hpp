#ifndef __CLOCK_INTERFACE_H__
#define __CLOCK_INTERFACE_H__

#include <stdint.h>

class ClockInterface
{
    public:
        virtual ~ClockInterface() {}
        virtual uint64_t getTimeUs() const = 0;
};

#endif // __CLOCK_INTERFACE_H__
