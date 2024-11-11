#pragma once

#include "VibrationObserver.hpp"

//! USB initialization
void usb_init();
//! USB task that needs to be called constantly by main()
void usb_task(uint64_t timeUs);
//! @returns pointer to the vibration observer for the USB host
VibrationObserver* get_usb_vibration_observer();