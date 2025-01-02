#pragma once

#include <string>

//! Abstract observer class for the transient overlay
class TransientOverlayObserver
{
    public:
        virtual ~TransientOverlayObserver() {}
        virtual void notify(uint8_t& message) = 0;
};