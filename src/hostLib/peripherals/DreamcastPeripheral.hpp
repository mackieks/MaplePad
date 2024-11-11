#pragma once

#include <stdint.h>
#include "PrioritizedTxScheduler.hpp"
#include "EndpointTxSchedulerInterface.hpp"
#include "Transmitter.hpp"

//! Base class for a connected Dreamcast peripheral
class DreamcastPeripheral : public Transmitter
{
    public:
        //! Constructor
        //! @param[in] name  Name that the child class defines which is used for logging
        //! @param[in] addr  This peripheral's address (mask bit)
        //! @param[in] fd  Function definition from the device info for this peripheral
        //! @param[in] scheduler  The transmission scheduler this peripheral is to add to
        //! @param[in] playerIndex  Player index of this peripheral [0,3]
        DreamcastPeripheral(const char* name,
                            uint8_t addr,
                            uint32_t fd,
                            std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                            uint32_t playerIndex) :
            mName(name),
            mEndpointTxScheduler(scheduler),
            mPlayerIndex(playerIndex),
            mAddr(addr),
            mFd(fd)
        {}

        //! Virtual destructor
        virtual ~DreamcastPeripheral()
        {}

        //! @param[in] subPeripheralIndex  Sub peripheral index [0,4]
        //! @returns the sub peripheral mask for the given sub peripheral index
        static inline uint8_t subPeripheralMask(int32_t subPeripheralIndex)
        {
            return SUB_PERIPHERAL_ADDR_START_MASK << subPeripheralIndex;
        }

        //! @param[in] subPeripheralMask  Sub peripheral mask
        //! @returns the index of the first sub peripheral mask that was matched
        static inline int32_t subPeripheralIndex(uint8_t subPeripheralMask)
        {
            uint8_t mask = SUB_PERIPHERAL_ADDR_START_MASK;
            for (uint32_t i = 0; i < MAX_SUB_PERIPHERALS; ++i, mask<<=1)
            {
                if (subPeripheralMask & mask)
                {
                    return i;
                }
            }
            return -1;
        }

        //! Get recipient address for a peripheral with given player index and address
        //! @param[in] playerIndex  Player index of peripheral [0,3]
        //! @param[in] addr  Peripheral's address (mask bit)
        //! @returns recipient address
        static inline uint8_t getRecipientAddress(uint32_t playerIndex, uint8_t addr)
        {
            return (playerIndex << 6) | addr;
        }

        //! @returns recipient address for this peripheral
        inline uint8_t getRecipientAddress() { return getRecipientAddress(mPlayerIndex, mAddr); }

        //! The task that DreamcastNode yields control to after this peripheral is detected
        //! @param[in] currentTimeUs  The current time in microseconds
        virtual void task(uint64_t currentTimeUs) = 0;

        //! @returns peripheral name
        inline const char* const getName() { return mName; }

        //! @returns the function definition of this peripheral
        inline const uint32_t& getFunctionDefinition() { return mFd; }

    public:
        //! The maximum number of sub peripherals that a main peripheral can handle
        static const uint32_t MAX_SUB_PERIPHERALS = 5;
        //! Main peripheral address mask
        static const uint8_t MAIN_PERIPHERAL_ADDR_MASK = 0x20;
        //! The first sub peripheral address
        static const uint8_t SUB_PERIPHERAL_ADDR_START_MASK = 0x01;

    protected:
        //! Unique name of this peripheral
        const char* const mName;
        //! Keeps all scheduled transmissions for the bus this peripheral is connected to
        std::shared_ptr<EndpointTxSchedulerInterface> mEndpointTxScheduler;
        //! Player index of this peripheral [0,3]
        const uint32_t mPlayerIndex;
        //! Address of this device
        const uint8_t mAddr;
        //! Function definition of this device
        const uint32_t mFd;
};
