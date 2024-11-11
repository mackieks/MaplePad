#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>

#include "hal/System/MutexInterface.hpp"
#include "hal/Usb/TtyParser.hpp"
#include "hal/Usb/CommandParser.hpp"

// Command structure: [whitespace]<command-char>[command]<\n>

//! Command parser for processing commands from a TTY stream
class UsbCdcTtyParser : public TtyParser
{
public:
    //! Constructor
    UsbCdcTtyParser(MutexInterface& m, char helpChar);
    //! Adds a command parser to my list of parsers - must be done before any other function called
    virtual void addCommandParser(std::shared_ptr<CommandParser> parser) final;
    //! Called from the process receiving characters on the TTY
    void addChars(const char* chars, uint32_t len);
    //! Called from the process handling maple bus execution
    virtual void process() final;

private:
    //! Max of 2 KB of memory to use for tty RX queue
    static const uint32_t MAX_QUEUE_SIZE = 2048;
    //! String of characters that are considered whitespace
    static const char* WHITESPACE_CHARS;
    //! String of characters that are considered end of line characters
    static const char* INPUT_EOL_CHARS;
    //! The singular character that is replaced in the RX list as the EOL char
    static const char RX_EOL_CHAR;
    //! String of characters that are treated as a backspace
    static const char* BACKSPACE_CHARS;
    //! Receive queue
    std::vector<char> mParserRx;
    //! Flag that is set to true if the last read character is an EOL (used to ignore further EOL)
    bool mLastIsEol;
    //! Mutex used to serialize addChars and process
    MutexInterface& mParserMutex;
    //! Flag when end of line detected on add
    std::atomic<bool> mCommandReady;
    //! The command character which prints help for all commands
    const char mHelpChar;
    //! Parsers that may handle data
    std::vector<std::shared_ptr<CommandParser>> mParsers;
    //! true when overflow in mParserRx
    bool mOverflowDetected;
};
