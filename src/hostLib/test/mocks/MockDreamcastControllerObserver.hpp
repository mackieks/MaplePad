#pragma once

#include "hal/Usb/DreamcastControllerObserver.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockDreamcastControllerObserver : public DreamcastControllerObserver
{
    public:
        MOCK_METHOD(void, setControllerCondition, (const ControllerCondition& controllerCondition), (override));

        MOCK_METHOD(void, controllerConnected, (), (override));

        MOCK_METHOD(void, controllerDisconnected, (), (override));

        MOCK_METHOD(void, setSecondaryControllerCondition,
            (const SecondaryControllerCondition& secondaryControllerCondition), (override));
};
