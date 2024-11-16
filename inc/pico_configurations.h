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
#define CTRL_PIN_C 16
#define CTRL_PIN_Z 17
#define OLED_PAGE_CYCLE_PIN 21
#define OLED_SEL_PIN 22
#define CTRL_PIN_SX 26
#define CTRL_PIN_SY 27
#define CTRL_PIN_LT 28
#define CTRL_PIN_RT 29 // Reserved for temp sensor ADC, needs to be bypassed on official picos

#define NUM_BUTTONS 11

typedef struct ButtonInfo_s {
    int pin;
    int buttonMask;
} ButtonInfo;

static const ButtonInfo ButtonInfos[NUM_BUTTONS] = {
    {0, 0x0004}, // A - 0
    {1, 0x0002}, // B - 1
    {4, 0x0400}, // X - 2
    {5, 0x0200}, // Y - 3
    {6, 0x0010}, // Up - 4
    {7, 0x0020}, // Down - 5
    {8, 0x0040}, // Left - 6
    {9, 0x0080}, // Right - 7
    {10, 0x0008}, // Start - 8
    {16, 0x0001}, // C - 9
    {17, 0x0100}  // Z - 10
};