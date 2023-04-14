#pragma once

#define NUM_STATES 40
#define NUM_SETBITS 64

typedef struct StateMachine_s {
  uint16_t NewState : 6;
  uint16_t Push : 1;
  uint16_t Error : 1;
  uint16_t Reset : 1;
  uint16_t End : 1;
  uint16_t SetBitsIndex : 6;
} StateMachine;

// The state machine table
// Pre-calculated responses for any byte we can recieve from Maple RX PIO
extern StateMachine Machine[NUM_STATES][256]; // 20Kb

// Bits to set indexed from StateMachine::SetBitsIndex
extern uint8_t SetBits[NUM_SETBITS][2]; // 128 bytes

// Function which builds above tables
void BuildStateMachineTables(void);
