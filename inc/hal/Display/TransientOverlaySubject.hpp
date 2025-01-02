#pragma once

#include <string>
#include <list>
#include <memory>
#include "TransientOverlayObserver.hpp"

class TransientOverlaySubject {
public:
    virtual ~TransientOverlaySubject() {}

    void attach(std::shared_ptr<TransientOverlayObserver> observer) {
        mObservers.push_front(observer);
    }

    void detach(std::shared_ptr<TransientOverlayObserver> observer) {
        mObservers.remove(observer);
    }

    void notify(uint8_t& message) {
        for(auto& o: mObservers) {
            o->notify(message);
        }
    }

private:
    std::list<std::shared_ptr<TransientOverlayObserver>> mObservers;
};