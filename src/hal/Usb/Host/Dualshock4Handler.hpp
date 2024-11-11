#pragma once

#include "VibrationObserver.hpp"
#include "HidHandler.hpp"

class Dualshock4Handler : public VibrationObserver, public HidHandler
{
public:
    inline Dualshock4Handler()
    {}

    virtual inline void vibrate(
        float frequency,
        float intensity,
        int8_t inclination,
        float duration) final
    {}

    virtual inline void handleReport(uint8_t const* report, uint16_t len) final
    {}
};
