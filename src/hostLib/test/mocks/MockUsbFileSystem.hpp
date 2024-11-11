#pragma once

#include "hal/Usb/UsbFileSystem.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockUsbFileSystem : public UsbFileSystem
{
    public:
        MOCK_METHOD(void, add, (UsbFile* file), (override));
        MOCK_METHOD(void, remove, (UsbFile* file), (override));
};
