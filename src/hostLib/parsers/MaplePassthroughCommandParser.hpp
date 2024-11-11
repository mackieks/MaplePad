#pragma once

#include "hal/Usb/CommandParser.hpp"

#include "PrioritizedTxScheduler.hpp"

#include <memory>

// Command structure: [whitespace]<command-char>[command]<\n>

//! Command parser for processing commands from a TTY stream
class MaplePassthroughCommandParser : public CommandParser
{
public:
    MaplePassthroughCommandParser(std::shared_ptr<PrioritizedTxScheduler>* schedulers,
                                  const uint8_t* senderAddresses,
                                  uint32_t numSenders);

    //! @returns the string of command characters this parser handles
    virtual const char* getCommandChars() final;

    //! Called when newline reached; submit command and reset
    virtual void submit(const char* chars, uint32_t len) final;

    //! Prints help message for this command
    virtual void printHelp() final;

private:
    std::shared_ptr<PrioritizedTxScheduler>* const mSchedulers;
    const uint8_t* const mSenderAddresses;
    const uint32_t mNumSenders;
};
