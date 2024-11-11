#include "EndpointTxScheduler.hpp"

EndpointTxScheduler::EndpointTxScheduler(
    std::shared_ptr<PrioritizedTxScheduler> prioritizedScheduler,
    uint8_t fixedPriority,
    uint8_t recipientAddr):
        mPrioritizedScheduler(prioritizedScheduler),
        mFixedPriority(fixedPriority),
        mRecipientAddr(recipientAddr)
{}

EndpointTxScheduler::~EndpointTxScheduler()
{}

uint32_t EndpointTxScheduler::add(uint64_t txTime,
                                  Transmitter* transmitter,
                                  uint8_t command,
                                  uint32_t* payload,
                                  uint8_t payloadLen,
                                  bool expectResponse,
                                  uint32_t expectedResponseNumPayloadWords,
                                  uint32_t autoRepeatUs,
                                  uint64_t autoRepeatEndTimeUs)
{
    MaplePacket packet({.command=command, .recipientAddr=mRecipientAddr}, payload, payloadLen);
    return mPrioritizedScheduler->add(mFixedPriority,
                                      txTime,
                                      transmitter,
                                      packet,
                                      expectResponse,
                                      expectedResponseNumPayloadWords,
                                      autoRepeatUs,
                                      autoRepeatEndTimeUs);
}

uint32_t EndpointTxScheduler::cancelById(uint32_t transmissionId)
{
    return mPrioritizedScheduler->cancelById(transmissionId);
}

uint32_t EndpointTxScheduler::cancelByRecipient(uint8_t recipientAddr)
{
    return mPrioritizedScheduler->cancelByRecipient(recipientAddr);
}

uint32_t EndpointTxScheduler::countRecipients(uint8_t recipientAddr)
{
    return mPrioritizedScheduler->countRecipients(recipientAddr);
}

uint32_t EndpointTxScheduler::cancelAll()
{
    return mPrioritizedScheduler->cancelAll();
}