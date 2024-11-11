#pragma once

#include "hal/Usb/DreamcastControllerObserver.hpp"
#include "hal/System/ClockInterface.hpp"
#include "ScreenData.hpp"
#include "hal/Usb/UsbFileSystem.hpp"

//! Contains data that is tied to a specific player
struct PlayerData
{
    const uint32_t playerIndex;
    DreamcastControllerObserver& gamepad;
    ScreenData& screenData;
    ClockInterface& clock;
    UsbFileSystem& fileSystem;

    PlayerData(uint32_t playerIndex,
               DreamcastControllerObserver& gamepad,
               ScreenData& screenData,
               ClockInterface& clock,
               UsbFileSystem& fileSystem) :
        playerIndex(playerIndex),
        gamepad(gamepad),
        screenData(screenData),
        clock(clock),
        fileSystem(fileSystem)
    {}
};