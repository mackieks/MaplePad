#pragma once

#include <string>
#include <vector>
#include <memory>
#include "TransientOverlayObserver.hpp"

class TransientOverlaySubject {
public:
    virtual ~TransientOverlaySubject() {}
    virtual void attach(std::shared_ptr<TransientOverlayObserver> observer) = 0;
    virtual void detach(std::shared_ptr<TransientOverlayObserver> observer) = 0;
    virtual void notify(const std::string& message) = 0;
};