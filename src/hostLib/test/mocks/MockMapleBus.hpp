#pragma once

#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/MapleBus/MaplePacket.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockMapleBus : public MapleBusInterface
{
    public:
        virtual bool write(const MaplePacket& packet,
                           bool autostartRead,
                           uint64_t readTimeoutUs=MAPLE_RESPONSE_TIMEOUT_US) override
        {
            return mockWrite(packet, autostartRead, readTimeoutUs);
        }

        MOCK_METHOD(
            bool,
            mockWrite,
            (
                const MaplePacket& packet,
                bool expectResponse,
                uint64_t readTimeoutUs
            )
        );

        MOCK_METHOD(Status, processEvents, (uint64_t currentTimeUs), (override));

        MOCK_METHOD(bool, isBusy, (), (override));

        MOCK_METHOD(bool, startRead, (uint64_t readTimeoutUs), (override));
};
