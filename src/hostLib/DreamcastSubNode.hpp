#pragma once

#include "DreamcastNode.hpp"

//! Handles communication for a Dreamcast sub node for a single bus.
class DreamcastSubNode : public DreamcastNode
{
    public:
        //! Constructor
        //! @param[in] addr  The address of this sub node
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  The player data passed to any connected peripheral
        DreamcastSubNode(uint8_t addr,
                         std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                         PlayerData playerData);

        //! Copy constructor
        DreamcastSubNode(const DreamcastSubNode& rhs);

        //! Inherited from DreamcastNode
        virtual inline void txStarted(std::shared_ptr<const Transmission> tx)
        {}

        //! Inherited from DreamcastNode
        virtual inline void txFailed(bool writeFailed,
                                     bool readFailed,
                                     std::shared_ptr<const Transmission> tx)
        {}

        //! Inherited from DreamcastNode
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx);

        //! Inherited from DreamcastNode
        virtual void task(uint64_t currentTimeUs);

        //! Called from the main node when a main peripheral disconnects. A main peripheral
        //! disconnecting should cause all sub peripherals to disconnect.
        virtual void mainPeripheralDisconnected();

        //! Called from the main node to update the connection state of peripherals on this sub node
        virtual void setConnected(bool connected, uint64_t currentTimeUs = 0);

    protected:
        //! Number of microseconds in between each info request when no peripheral is detected
        static const uint32_t US_PER_CHECK = 16000;
        //! Detected peripheral connection state
        bool mConnected;
        //! ID of the device info request auto reload transmission this object added to the schedule
        int64_t mScheduleId;

};
