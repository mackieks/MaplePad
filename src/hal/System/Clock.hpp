#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "hal/System/ClockInterface.hpp"

class Clock : public ClockInterface
{
    public:
        virtual uint64_t getTimeUs() const final;
};

#endif // __CLOCK_H__
