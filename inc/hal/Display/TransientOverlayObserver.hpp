#pragma once

#include <string>

//! Abstract observer class for the transient overlay
class TransientOverlayObserver
{
    public:
        virtual ~TransientOverlayObserver() {}
        virtual void update(const std::string& message) = 0;
};