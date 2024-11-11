#include "DreamcastGun.hpp"

DreamcastGun::DreamcastGun(uint8_t addr,
                           uint32_t fd,
                           std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                           PlayerData playerData) :
    DreamcastPeripheral("gun", addr, fd, scheduler, playerData.playerIndex)
{}

DreamcastGun::~DreamcastGun()
{}

void DreamcastGun::task(uint64_t currentTimeUs)
{}

void DreamcastGun::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastGun::txFailed(bool writeFailed,
                            bool readFailed,
                            std::shared_ptr<const Transmission> tx)
{}

void DreamcastGun::txComplete(std::shared_ptr<const MaplePacket> packet,
                              std::shared_ptr<const Transmission> tx)
{}
