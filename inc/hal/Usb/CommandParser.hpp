#pragma once

#include <stdint.h>
#include <vector>
#include <memory>

#include "hal/System/MutexInterface.hpp"

// Command structure: [whitespace]<command-char>[command]<\n>

//! Command parser for processing commands from a TTY stream
class CommandParser
{
public:
    virtual ~CommandParser() {}

    //! @returns the string of command characters this parser handles
    virtual const char* getCommandChars() = 0;

    //! Called when newline reached; submit command and reset
    virtual void submit(const char* chars, uint32_t len) = 0;

    //! Prints help message for this command
    virtual void printHelp() = 0;
};
