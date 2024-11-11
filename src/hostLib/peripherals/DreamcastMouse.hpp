#pragma once

#include "DreamcastPeripheral.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast mouse peripheral
class DreamcastMouse : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] fd  Function definition from the device info for this peripheral
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this mouse device
        DreamcastMouse(uint8_t addr,
                       uint32_t fd,
                       std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                       PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastMouse();

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Called when transmission has been sent
        //! @param[in] tx  The transmission that was sent
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

    public:
        //! Function code for mouse
        static const uint32_t FUNCTION_CODE = DEVICE_FN_MOUSE;

    private:
};
