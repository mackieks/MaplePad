#include "DreamcastKeyboard.hpp"

DreamcastKeyboard::DreamcastKeyboard(uint8_t addr,
                                     uint32_t fd,
                                     std::shared_ptr<EndpointTxSchedulerInterface> scheduler,
                                     PlayerData playerData) :
    DreamcastPeripheral("keyboard", addr, fd, scheduler, playerData.playerIndex)
{}

DreamcastKeyboard::~DreamcastKeyboard()
{}

void DreamcastKeyboard::task(uint64_t currentTimeUs)
{}

void DreamcastKeyboard::txStarted(std::shared_ptr<const Transmission> tx)
{}

void DreamcastKeyboard::txFailed(bool writeFailed,
                                 bool readFailed,
                                 std::shared_ptr<const Transmission> tx)
{}

void DreamcastKeyboard::txComplete(std::shared_ptr<const MaplePacket> packet,
                                   std::shared_ptr<const Transmission> tx)
{}
