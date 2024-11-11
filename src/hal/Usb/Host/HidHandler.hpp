#pragma once

#include <stdint.h>

class HidHandler
{
public:
    virtual ~HidHandler() {}

    virtual void handleReport(uint8_t const* report, uint16_t len) = 0;
};