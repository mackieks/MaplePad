#pragma once

#include <stdint.h>

//! Structure used to unpack an 8-byte controller condition package (little endian assumed)
typedef struct controller_condition_s
{
    uint8_t l; //!< 0: fully released; 255: fully pressed

    uint8_t r; //!< 0: fully released; 255: fully pressed

    // Digital bits:
    // 0: pressed
    // 1: released
    unsigned z:1;
    unsigned y:1;
    unsigned x:1;
    unsigned d:1;
    unsigned upb:1;
    unsigned downb:1;
    unsigned leftb:1;
    unsigned rightb:1;

    unsigned c:1;
    unsigned b:1;
    unsigned a:1;
    unsigned start:1;
    unsigned up:1;
    unsigned down:1;
    unsigned left:1;
    unsigned right:1;

    uint8_t rAnalogUD; //!< 0: up; 128: neutral; 255: down

    uint8_t rAnalogLR; //!< 0: up; 128: neutral; 255: down

    uint8_t lAnalogUD; //!< 0: up; 128: neutral; 255: down

    uint8_t lAnalogLR; //!< 0: left; 128: neutral; 255: right
}__attribute__((packed)) controller_condition_t;

#define NEUTRAL_CONTROLLER_CONDITION {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 128, 128, 128, 128}

//! Button condition bits found in the VMU timer function (least significant byte)
typedef struct vmu_timer_condition_s
{
    // Digital bits:
    // 0: pressed
    // 1: released
    unsigned up:1;
    unsigned down:1;
    unsigned left:1;
    unsigned right:1;
    unsigned a:1;
    unsigned b:1;
    unsigned c:1;
    unsigned start:1;
} __attribute__((packed)) vmu_timer_condition_t;

#define NEUTRAL_VMU_TIMER_CONDITION {1, 1, 1, 1, 1, 1, 1, 1}
