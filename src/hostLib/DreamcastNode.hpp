#pragma once

#include "DreamcastPeripheral.hpp"

#include "dreamcast_constants.h"
#include "PlayerData.hpp"
#include "DreamcastController.hpp"
#include "DreamcastStorage.hpp"
#include "DreamcastScreen.hpp"
#include "DreamcastTimer.hpp"
#include "DreamcastVibration.hpp"
#include "DreamcastMicrophone.hpp"
#include "DreamcastArGun.hpp"
#include "DreamcastKeyboard.hpp"
#include "DreamcastGun.hpp"
#include "DreamcastMouse.hpp"
#include "DreamcastExMedia.hpp"
#include "DreamcastCamera.hpp"
#include "EndpointTxSchedulerInterface.hpp"
#include "Transmitter.hpp"
#include "utils.h"

#include <stdint.h>
#include <vector>
#include <memory>
#include <bitset>

//! Base class for an addressable node on a Maple Bus
class DreamcastNode : public Transmitter
{
    public:
        //! Virtual destructor
        virtual ~DreamcastNode() {}

        //! Called periodically for this node to execute tasks for the given point in time
        //! @param[in] currentTimeUs  The current time in microseconds
        virtual void task(uint64_t currentTimeUs) = 0;

        //! @returns this node's address
        inline uint8_t getAddr() { return mAddr; }

        //! @returns recipient address for this node
        inline uint8_t getRecipientAddress()
        {
            return DreamcastPeripheral::getRecipientAddress(mPlayerData.playerIndex, mAddr);
        }

    protected:
        //! Main constructor with scheduler
        DreamcastNode(uint8_t addr,
                      std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                      PlayerData playerData) :
            mAddr(addr), mEndpointTxScheduler(scheduler), mPlayerData(playerData), mPeripherals()
        {}

        //! Copy constructor
        DreamcastNode(const DreamcastNode& rhs) :
            mAddr(rhs.mAddr),
            mEndpointTxScheduler(rhs.mEndpointTxScheduler),
            mPlayerData(rhs.mPlayerData),
            mPeripherals()
        {
            mPeripherals = rhs.mPeripherals;
        }

        //! Run all peripheral tasks
        //! @param[in] currentTimeUs  The current time in microseconds
        void handlePeripherals(uint64_t currentTimeUs)
        {
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end();
                 ++iter)
            {
                (*iter)->task(currentTimeUs);
            }
        }

        //! Factory function which generates peripheral objects for the given function code mask
        //! @param[in] deviceInfoPayload  The payload within the received device info packet
        //! @returns mask items not handled
        virtual uint32_t peripheralFactory(const std::vector<uint32_t>& deviceInfoPayload)
        {
            uint32_t functionCode = 0;
            if (deviceInfoPayload.size() > 3)
            {
                // First word is function code, next 3 should be function definitions
                functionCode = deviceInfoPayload[0];
                std::vector<uint32_t> functionDefinitions(deviceInfoPayload.cbegin() + 1,
                                                          deviceInfoPayload.cbegin() + 4);

                mPeripherals.clear();

                peripheralFactoryCheck<DreamcastController>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastStorage>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastScreen>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastTimer>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastVibration>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastMicrophone>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastArGun>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastKeyboard>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastGun>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastMouse>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastExMedia>(functionCode, functionDefinitions);
                peripheralFactoryCheck<DreamcastCamera>(functionCode, functionDefinitions);

                // TODO: handle other peripherals here
            }

            return functionCode;
        }

        //! Prints all peripheral names
        inline void debugPrintPeripherals()
        {
            for (std::vector<std::shared_ptr<DreamcastPeripheral>>::iterator iter = mPeripherals.begin();
                 iter != mPeripherals.end();
                 ++iter)
            {
                if (iter != mPeripherals.begin())
                {
                    DEBUG_PRINT(", ");
                }
                DEBUG_PRINT("%s-%08lX",
                            (*iter)->getName(),
                            (long unsigned int)(*iter)->getFunctionDefinition());
            }
        }

    private:
        //! Default constructor - not implemented
        DreamcastNode() = delete;

        //! Adds a peripheral to peripheral vector
        //! @param[in,out] functionCode  Mask which contains function codes to check; returns mask
        //!                              minus peripheral that was added, if any
        //! @param[in,out] functionDefinitions  Remaining function definitions; the consumed
        //!                                     function definition (if any) will be removed
        template <class PeripheralClass>
        inline void peripheralFactoryCheck(uint32_t& functionCode,
                                           std::vector<uint32_t>& functionDefinitions)
        {
            if (functionCode & PeripheralClass::FUNCTION_CODE)
            {
                // Get bit mask which contains only the bits to the left of the function code
                std::bitset<32> leftBits(functionCode & ~(PeripheralClass::FUNCTION_CODE | (PeripheralClass::FUNCTION_CODE - 1)));
                // cnt is essentially the index in functionDefinitions to go to
                size_t cnt = leftBits.count();
                // Get to the target function definition, save value, and delete it
                uint32_t fd = 0;
                if (cnt < functionDefinitions.size())
                {
                    std::vector<uint32_t>::iterator iter = functionDefinitions.begin() + cnt;
                    fd = *iter;
                    functionDefinitions.erase(iter);
                }
                // Create the peripheral!
                mPeripherals.push_back(std::make_shared<PeripheralClass>(mAddr, fd, mEndpointTxScheduler, mPlayerData));
                functionCode &= ~PeripheralClass::FUNCTION_CODE;
            }
        }

    protected:
        //! Maximum number of players
        static const uint32_t MAX_NUM_PLAYERS = 4;
        //! Address of this node
        const uint8_t mAddr;
        //! Keeps all scheduled transmissions for my bus
        const std::shared_ptr<EndpointTxSchedulerInterface> mEndpointTxScheduler;
        //! Player data on this node
        PlayerData mPlayerData;
        //! The connected peripherals addressed to this node (usually 0 to 3 items)
        std::vector<std::shared_ptr<DreamcastPeripheral>> mPeripherals;
};