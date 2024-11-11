#include "DreamcastTimer.hpp"
#include <memory.h>

DreamcastTimer::DreamcastTimer(uint8_t addr,
                               uint32_t fd,
                               std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                               PlayerData playerData) :
    DreamcastPeripheral("timer", addr, fd, scheduler, playerData.playerIndex),
    mGamepad(playerData.gamepad),
    mButtonStatusId(0)
{
    // Poll only the upper VMU button states
    if (addr & SUB_PERIPHERAL_ADDR_START_MASK)
    {
        uint32_t payload = FUNCTION_CODE;
        mButtonStatusId = mEndpointTxScheduler->add(
            PrioritizedTxScheduler::TX_TIME_ASAP,
            this,
            COMMAND_GET_CONDITION,
            &payload,
            1,
            true,
            2,
            BUTTON_POLL_PERIOD_US);
    }
}

DreamcastTimer::~DreamcastTimer()
{}

void DreamcastTimer::task(uint64_t currentTimeUs)
{}

void DreamcastTimer::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastTimer::txFailed(bool writeFailed,
                              bool readFailed,
                              std::shared_ptr<const Transmission> tx)
{}

void DreamcastTimer::txComplete(std::shared_ptr<const MaplePacket> packet,
                                std::shared_ptr<const Transmission> tx)
{
    if (tx->transmissionId == mButtonStatusId
        && packet->frame.command == COMMAND_RESPONSE_DATA_XFER
        && packet->payload.size() >= 2)
    {
        // Set controller!
        const uint8_t cond = packet->payload[1] >> COND_RIGHT_SHIFT;
        DreamcastControllerObserver::SecondaryControllerCondition secondaryCondition;
        memcpy(&secondaryCondition, &cond, 1);
        mGamepad.setSecondaryControllerCondition(secondaryCondition);
    }
}
