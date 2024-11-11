#pragma once

#include "hal/System/ClockInterface.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockClock : public ClockInterface
{
    public:
        MOCK_METHOD(uint64_t, getTimeUs, (), (const, override));
};
