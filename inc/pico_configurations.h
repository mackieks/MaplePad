#pragma once

#include <stdint.h>

//!The "wired" controller GPIOs for MaplePad. The DreamcastController constructor handles enabling controller
//inputs which results in those inputs being masked when sent to Maplebus.
#define CTRL_PIN_A 0
#define CTRL_PIN_B 1
#define CTRL_PIN_X 4
#define CTRL_PIN_Y 5
#define CTRL_PIN_DU 6
#define CTRL_PIN_DD 7
#define CTRL_PIN_DL 8
#define CTRL_PIN_DR 9
#define CTRL_PIN_START 10
#define CTRL_PIN_RUMBLE 15
//#define CTRL_PIN_C 16
//#define CTRL_PIN_Z 17
#define OLED_PAGE_CYCLE_PIN 21
#define OLED_SEL_PIN 22
#define CTRL_PIN_SX 26
#define CTRL_PIN_SY 27
#define CTRL_PIN_LT 28
#define CTRL_PIN_RT 29 // Reserved for temp sensor ADC, needs to be bypassed on official picos

#define NUM_BUTTONS 9

typedef struct ButtonInfo_s {
    int pin;
} ButtonInfo;

static const ButtonInfo ButtonInfos[NUM_BUTTONS] = {
    {CTRL_PIN_A}, // A - 0
    {CTRL_PIN_B}, // B - 1
    {CTRL_PIN_X}, // X - 2
    {CTRL_PIN_Y}, // Y - 3
    {CTRL_PIN_DU}, // Up - 4
    {CTRL_PIN_DD}, // Down - 5
    {CTRL_PIN_DL}, // Left - 6
    {CTRL_PIN_DR}, // Right - 7
    {CTRL_PIN_START} // Start - 8
    //{CTRL_PIN_C}, // C - 9
    //{CTRL_PIN_Z}  // Z - 10
};