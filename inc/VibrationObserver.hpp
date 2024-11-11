#pragma once

#include <stdint.h>

class VibrationObserver
{
public:
    virtual ~VibrationObserver() {}

    //! Activate vibration
    //! @param[in] frequency  The sine-wave frequency to vibrate
    //! @param[in] intensity  Intensity between 0.0 and 1.0
    //! @param[in] inclination  Inclination: -1 for ramp down, 0 for constant, 1 for ramp up
    //! @param[in] duration  The total vibration duration in seconds; 0.0 means no stop duration
    //!                      specified
    virtual void vibrate(float frequency, float intensity, int8_t inclination, float duration) = 0;
};
