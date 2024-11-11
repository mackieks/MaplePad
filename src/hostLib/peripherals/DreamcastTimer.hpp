#pragma once

#include "DreamcastPeripheral.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast timer peripheral
class DreamcastTimer : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] fd  Function definition from the device info for this peripheral
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this timer device
        DreamcastTimer(uint8_t addr,
                       uint32_t fd,
                       std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                       PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastTimer();

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
        //! Function code for timer
        static const uint32_t FUNCTION_CODE = DEVICE_FN_TIMER;
        //! Polling period for the upper VMU button states
        static const uint32_t BUTTON_POLL_PERIOD_US = 50000;
        //! Number of bits to shift the condition word to the right
        static const uint8_t COND_RIGHT_SHIFT = 24;

    private:
        //! Gamepad to send secondary status to
        DreamcastControllerObserver& mGamepad;
        //! Transmit ID of button status message
        uint32_t mButtonStatusId;
};
