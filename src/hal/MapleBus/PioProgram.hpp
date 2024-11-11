#pragma once

#include "hardware/pio.h"

class PioProgram
{
    public:
        inline PioProgram(pio_hw_t* pio, const pio_program_t *program) :
            mPio(pio),
            mProgram(program),
            mProgramOffset(pio_add_program(pio, program))
        {}

        pio_hw_t* const mPio;
        const pio_program_t* const mProgram;
        const uint mProgramOffset;
};
