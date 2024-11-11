#include "DreamcastCamera.hpp"

DreamcastCamera::DreamcastCamera(uint8_t addr,
                                 uint32_t fd,
                                 std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                 PlayerData playerData) :
    DreamcastPeripheral("camera", addr, fd, scheduler, playerData.playerIndex)
{}

DreamcastCamera::~DreamcastCamera()
{}

void DreamcastCamera::task(uint64_t currentTimeUs)
{}

void DreamcastCamera::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastCamera::txFailed(bool writeFailed,
                               bool readFailed,
                               std::shared_ptr<const Transmission> tx)
{}

void DreamcastCamera::txComplete(std::shared_ptr<const MaplePacket> packet,
                                 std::shared_ptr<const Transmission> tx)
{}
