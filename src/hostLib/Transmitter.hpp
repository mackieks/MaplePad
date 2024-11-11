#pragma once

#include <stdint.h>
#include <memory>

struct Transmission;
struct MaplePacket;

class Transmitter
{
public:
    Transmitter() {}
    virtual ~Transmitter() {}

    //! Called when transmission has started to be sent
    //! @param[in] tx  The transmission that was sent
    virtual void txStarted(std::shared_ptr<const Transmission> tx) = 0;

    //! Called when transmission failed
    //! @param[in] writeFailed  Set to true iff TX failed because write failed
    //! @param[in] readFailed  Set to true iff TX failed because read failed
    //! @param[in] tx  The transmission that failed
    virtual void txFailed(bool writeFailed,
                          bool readFailed,
                          std::shared_ptr<const Transmission> tx) = 0;

    //! Called when a transmission is complete
    //! @param[in] packet  The packet received or nullptr if this was write only transmission
    //! @param[in] tx  The transmission that triggered this data
    virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                            std::shared_ptr<const Transmission> tx) = 0;
};
