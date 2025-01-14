#pragma once

#include <stdint.h>
#include <memory>
#include <map>

#include "hal/MapleBus/MapleBusInterface.hpp"
#include "hal/MapleBus/MaplePacket.hpp"
#include "DreamcastPeripheral.hpp"

#include "pico/platform.h"

namespace client
{

class DreamcastMainPeripheral : public DreamcastPeripheral
{
public:
    typedef void (*PlayerIndexChangedFn)(int16_t idx);

public:
    DreamcastMainPeripheral(std::shared_ptr<MapleBusInterface> bus,
                            uint8_t addr,
                            uint8_t regionCode,
                            uint8_t connectionDirectionCode,
                            const char* descriptionStr,
                            const char* producerStr,
                            const char* versionStr,
                            float standbyCurrentmA,
                            float maxCurrentmA);

    DreamcastMainPeripheral(std::shared_ptr<MapleBusInterface> bus,
                            uint8_t addr,
                            uint8_t regionCode,
                            uint8_t connectionDirectionCode,
                            const char* descriptionStr,
                            const char* versionStr,
                            float standbyCurrentmA,
                            float maxCurrentmA);

    DreamcastMainPeripheral() = delete;

    //! Add a sub-peripheral to this peripheral (only valid of main peripheral)
    void addSubPeripheral(std::shared_ptr<DreamcastPeripheral> subPeripheral);

    //! Removes a previously added sub-peripheral
    //! @param[in] addr  The address of the sub-peripheral to remove
    bool removeSubPeripheral(uint8_t addr);

    //! Handle packet that is meant for me
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    //! Deliver given packet to target peripheral
    //! @param[in] in  The packet read from the Maple Bus
    //! @param[out] out  The packet to write to the Maple Bus when true is returned
    //! @return true iff the packet was dispensed and handled
    bool dispensePacket(const MaplePacket& in, MaplePacket& out);

    //! Called when player index changed, timeout occurred, or reset command received
    virtual void reset() final;

    //! Must be called periodically to process communication
    //! @param[in] currentTimeUs  The current system time in microseconds
    void task(uint64_t currentTimeUs);

    //! @returns player index [0,3] if set or -1 if not set
    int16_t getPlayerIndex();

    //! Set the player index changed callback
    //! @param[in] fn  Callback function pointer
    void setPlayerIndexChangedCb(PlayerIndexChangedFn fn);

    //! @returns number of times something has been read on the bus, destined to this peripheral
    inline uint32_t getReadCount() { return mReadCount; }

    //! Resets the read count
    inline void resetReadCount() { mReadCount = 0; }

    //! Allows bus communication to be processed
    inline void allowConnection() { mIsConnectionAllowed = true; }

    //! Disallows bus communication to be processed
    inline void disallowConnection() { mIsConnectionAllowed = false; }

    //! @returns true iff currently allowing communication on bus
    inline bool isConnectionAllowed() { return mIsConnectionAllowed; }

private:
    //! Set player index received from interface
    void setPlayerIndex(uint8_t idx);

public:
    //! Maple Bus read timeout in microseconds
    static const uint64_t READ_TIMEOUT_US = 1000000;

private:
    //! The bus this main peripheral is connected to
    const std::shared_ptr<MapleBusInterface> mBus;
    //! True when peripheral reacts to communication on bus (default: true)
    bool mIsConnectionAllowed;
    //! The current player index detected [0,3] or -1 if not set
    int16_t mPlayerIndex;
    //! All of the sub-peripherals attached to this main peripheral
    std::map<uint8_t, std::shared_ptr<DreamcastPeripheral>> mSubPeripherals;
    //! The sender address of the last received packet
    uint8_t mLastSender;
    //! Output packet buffer data
    MaplePacket mPacketOut;
    //! Last successfully sent packet, used for resend requests from host
    MaplePacket mLastPacketOut;
    //! Initialized to false and set to true once the first packet is sent
    bool mPacketSent;
    //! Input packet buffer
    MaplePacket mPacketIn;
    //! Callback for player index changed event
    PlayerIndexChangedFn mPlayerIndexChangedCb;
    //! Number of times something has been read on the bus, destined to this peripheral
    uint32_t mReadCount;
};

}
