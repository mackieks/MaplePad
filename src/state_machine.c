#include <string.h>
#include "pico/stdlib.h"
#include "state_machine.h"

// Simple state machine
// This part of the code encodes the basic state machine states for recieving Maple packets
// We will later use this to precalculate the lookup table for dealing with multiple transitions at once
// The tables we create here could be thrown away once the combined state machine table is built

enum {
  STATUS_NONE = -1,
  STATUS_START,
  STATUS_END,
  STATUS_PUSHBIT0,
  STATUS_PUSHBIT1,
  STATUS_PUSHBIT2,
  STATUS_PUSHBIT3,
  STATUS_PUSHBIT4,
  STATUS_PUSHBIT5,
  STATUS_PUSHBIT6,
  STATUS_PUSHBIT7,
  STATUS_BITSET = 0x80,
};

typedef struct SimpleState_s {
  int Next[4];
  int Status;
} SimpleState;

static SimpleState States[NUM_STATES];
static int NumStates = 0;

static int NewState(int Expected) {
  int New = NumStates++;
  SimpleState *s = &States[New];
  memset(s, 0xFF, sizeof(*s));
  s->Next[Expected] = New;
  return New;
}

static int ExpectState(int ParentState, int Expected) {
  int New = NewState(Expected);
  States[ParentState].Next[Expected] = New;
  return New;
}

static int ExpectStateWithStatus(int ParentState, int Expected, uint Status) {
  int New = NewState(Expected);
  States[New].Status = Status;
  States[ParentState].Next[Expected] = New;
  return New;
}

static int ExpectStateTwoParents(int ParentState, int OtherParentState, int Expected) {
  int New = NewState(Expected);
  States[ParentState].Next[Expected] = New;
  States[OtherParentState].Next[Expected] = New;
  return New;
}

static void BuildBasicStates() {
  // The transitions we expect to happen for a valid stream
  // 0b10 is Maple bus pin 5 high
  // 0b01 is Maple bus pin 1 high
  // Using info from http://mc.pp.se/dc/maplewire.html

  // Start (11 states)
  int Prev = NewState(0b11);
  Prev = ExpectState(Prev, 0b10);
  Prev = ExpectState(Prev, 0b00);
  Prev = ExpectState(Prev, 0b10);
  Prev = ExpectState(Prev, 0b00);
  Prev = ExpectState(Prev, 0b10);
  Prev = ExpectState(Prev, 0b00);
  Prev = ExpectState(Prev, 0b10);
  Prev = ExpectState(Prev, 0b00);
  Prev = ExpectState(Prev, 0b10);
  Prev = ExpectStateWithStatus(Prev, 0b11, STATUS_START);

  // Byte (6*4 = 24 states)
  // We encode each bit seperately so we don't need to do any shifting when recieving
  int PossibleEnd;
  int Option = Prev;
  int StartByte = NumStates;
  for (int i = 0; i < 4; i++) {
    Prev = ExpectStateTwoParents(Option, Prev, 0b01);
    Option = ExpectStateWithStatus(Prev, 0b11, (STATUS_PUSHBIT0 + i * 2) | STATUS_BITSET);
    Prev = ExpectStateWithStatus(Prev, 0b00, (STATUS_PUSHBIT0 + i * 2));
    if (i == 0) {
      PossibleEnd = Option;
    }

    Prev = ExpectStateTwoParents(Option, Prev, 0b10);
    Option = ExpectStateWithStatus(Prev, 0b11, (STATUS_PUSHBIT1 + i * 2) | STATUS_BITSET);
    Prev = ExpectStateWithStatus(Prev, 0b00, (STATUS_PUSHBIT1 + i * 2));

    if (i == 3) {
      // Loop back around
      States[Option].Next[0b01] = StartByte;
      States[Prev].Next[0b01] = StartByte;
    }
  }

  // End (5 states)
  Prev = ExpectState(PossibleEnd, 0b01);
  Prev = ExpectState(Prev, 0b00);
  // Signal end now. We need to be at least 4 transitions back from real end as PIO only pushes byte (4 transitions) at a time
  // If we were closer to the end then we could be sitting in idle waiting for a transitions and we wouldn't be replying
  // However if end was very long (for some reason) could mean we reply while end is still happening
  Prev = ExpectStateWithStatus(Prev, 0b01, STATUS_END);
  Prev = ExpectState(Prev, 0b00);
  Prev = ExpectState(Prev, 0b01);
  States[Prev].Next[0b11] = 0;
  assert(NumStates == NUM_STATES);
}

// Combined state machine
// Produces the table we will use for recieving
// Uses the simple state machine to precalculate a response for every possible byte we could get from Maple RX PIO

StateMachine Machine[NUM_STATES][256]; // 20Kb
uint8_t SetBits[NUM_SETBITS][2];       // 128 bytes
static int SetBitsEntries = 0;

static int FindOrAddSetBits(uint8_t CurrentByte, uint8_t NextByte) {
  for (int i = 0; i < SetBitsEntries; i++) {
    if (SetBits[i][0] == CurrentByte && SetBits[i][1] == NextByte) {
      return i;
    }
  }
  int NewEntry = SetBitsEntries++;
  assert(NewEntry < NUM_SETBITS);
  SetBits[NewEntry][0] = CurrentByte;
  SetBits[NewEntry][1] = NextByte;
  return NewEntry;
}

void BuildStateMachineTables() {
  BuildBasicStates();

  // For any byte we can recieve (from Maple RX PIO) in any starting state pre-calculate a response
  for (int StartingState = 0; StartingState < NUM_STATES; StartingState++) {
    for (int ByteFromMapleRXPIO = 0; ByteFromMapleRXPIO < 256; ByteFromMapleRXPIO++) {
      StateMachine M = {0};
      int State = StartingState;
      int Transitions = ByteFromMapleRXPIO;
      int LastState = State;
      uint8_t DataBytes[2] = {0, 0};
      uint CurrentDataByte = 0;
      for (int i = 0; i < 4; i++) {
        State = States[State].Next[(Transitions >> 6) & 3];
        if (State < 0) {
          M.Error = 1;
          State = 0;
        }
        // Ideally we should always have a transition as that's what Maple RX PIO is doing but they seem to happen
        if (State != LastState) {
          uint Status = States[State].Status;
          if (Status & STATUS_BITSET) {
            // Data recieved most significant bit first
            DataBytes[CurrentDataByte] |= (1 << (7 - ((Status & ~STATUS_BITSET) - STATUS_PUSHBIT0)));
          }
          switch (States[State].Status & ~STATUS_BITSET) {
          case STATUS_START:
            M.Reset = 1;
            break;
          case STATUS_END:
            M.End = 1;
            break;
          case STATUS_PUSHBIT7: // Last bit of current byte
            M.Push = 1;
            CurrentDataByte = 1;
            break;
          }
          LastState = State;
        }
        Transitions <<= 2;
      }
      M.NewState = State;
      M.SetBitsIndex = FindOrAddSetBits(DataBytes[0], DataBytes[1]);
      Machine[StartingState][ByteFromMapleRXPIO] = M;
    }
  }
}