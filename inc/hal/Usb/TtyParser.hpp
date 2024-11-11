#pragma once

#include <stdint.h>
#include "hal/Usb/CommandParser.hpp"
#include "hal/System/MutexInterface.hpp"

//! Command parser for processing commands from a TTY stream
class TtyParser
{
public:
    //! Virtual destructor
    virtual ~TtyParser() {}
    //! Adds a command parser to my list of parsers - must be done before any other function called
    virtual void addCommandParser(std::shared_ptr<CommandParser> parser) = 0;
    //! Called from the process handling maple bus execution
    virtual void process() = 0;
};

TtyParser* usb_cdc_create_parser(MutexInterface* m, char helpChar);
