#pragma once

#include "DreamcastPeripheral.hpp"
#include "hal/Usb/DreamcastControllerObserver.hpp"
#include "PlayerData.hpp"

//! Handles communication with the Dreamcast controller peripheral
class DreamcastController : public DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address
        //! @param[in] fd  Function definition from the device info for this peripheral
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerData  Data tied to player which controls this controller
        DreamcastController(uint8_t addr,
                            uint32_t fd,
                            std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                            PlayerData playerData);

        //! Virtual destructor
        virtual ~DreamcastController();

        //! Inherited from DreamcastPeripheral
        virtual void task(uint64_t currentTimeUs) final;

        //! Inherited from DreamcastPeripheral
        virtual void txStarted(std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx) final;

        //! Inherited from DreamcastPeripheral
        virtual void txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx) final;

    public:
        //! Function code for controller
        static const uint32_t FUNCTION_CODE = DEVICE_FN_CONTROLLER;

    private:
        //! Time between each controller state poll (in microseconds)
        static const uint32_t US_PER_CHECK = 16000;
        //! The gamepad to write button presses to
        DreamcastControllerObserver& mGamepad;
        //! True iff the controller is waiting for data
        bool mWaitingForData;
        //! Initialized to true and set to false in task()
        bool mFirstTask;
        //! ID of the get condition transmission
        uint32_t mConditionTxId;
};
