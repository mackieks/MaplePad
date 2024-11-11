#include "DreamcastController.hpp"
#include "dreamcast_constants.h"
#include <string.h>


DreamcastController::DreamcastController(uint8_t addr,
                                         uint32_t fd,
                                         std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                         PlayerData playerData) :
    DreamcastPeripheral("controller", addr, fd, scheduler, playerData.playerIndex),
    mGamepad(playerData.gamepad),
    mWaitingForData(false),
    mFirstTask(true),
    mConditionTxId(0)
{
    mGamepad.controllerConnected();
}

DreamcastController::~DreamcastController()
{
    mGamepad.controllerDisconnected();
}

void DreamcastController::txStarted(std::shared_ptr<const Transmission> tx)
{
    if (mConditionTxId != 0 && tx->transmissionId == mConditionTxId)
    {
        mWaitingForData = true;
    }
}

void DreamcastController::txFailed(bool writeFailed,
                                   bool readFailed,
                                   std::shared_ptr<const Transmission> tx)
{
    if (mConditionTxId != 0 && tx->transmissionId == mConditionTxId)
    {
        mWaitingForData = false;
    }
}

void DreamcastController::txComplete(std::shared_ptr<const MaplePacket> packet,
                                     std::shared_ptr<const Transmission> tx)
{
    if (mWaitingForData && packet != nullptr)
    {
        mWaitingForData = false;

        if (packet->frame.command == COMMAND_RESPONSE_DATA_XFER
            && packet->payload.size() >= 3
            && packet->payload[0] == DEVICE_FN_CONTROLLER)
        {
            // Handle condition data
            DreamcastControllerObserver::ControllerCondition controllerCondition;
            memcpy(&controllerCondition, &packet->payload[1], 2 * sizeof(uint32_t));
            mGamepad.setControllerCondition(controllerCondition);
        }
    }
}

void DreamcastController::task(uint64_t currentTimeUs)
{
    if (mFirstTask)
    {
        mFirstTask = false;
        uint32_t payload[] = {DEVICE_FN_CONTROLLER};
        uint64_t txTime = PrioritizedTxScheduler::computeNextTimeCadence(currentTimeUs, US_PER_CHECK);
        mConditionTxId = mEndpointTxScheduler->add(
            txTime,
            this,
            COMMAND_GET_CONDITION,
            payload,
            1,
            true,
            3,
            US_PER_CHECK);
    }
}
