#ifndef __USB_DESCRITORS_H__
#define __USB_DESCRITORS_H__

#include "configuration.h"

// Going in reverse order because the host seems to usually enumerate the highest value first
#define ITF_NUM_GAMEPAD(numGamepads, idx) (numGamepads - idx - 1)

#define NUMBER_OF_GAMEPADS (4)

// For mass storage device
#define ITF_NUM_MSC(numGamepads) (numGamepads)

#define ITF_NUM_CDC(numGamepads) (numGamepads + 1)
#define ITF_NUM_CDC_DATA(numGamepads) (numGamepads + 2)
#define ITF_COUNT(numGamepads) (numGamepads + 3)

#endif // __USB_DESCRITORS_H__
