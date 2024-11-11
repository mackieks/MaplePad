#include "UsbGamepad.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"

#include "utils.h"

UsbGamepad::UsbGamepad(uint8_t interfaceId, uint8_t reportId) :
  interfaceId(interfaceId),
  reportId(reportId),
  currentDpad(),
  currentButtons(0),
  buttonsUpdated(true)
{
  updateAllReleased();
}

bool UsbGamepad::isButtonPressed()
{
  return (
    currentDpad[DPAD_UP]
    || currentDpad[DPAD_DOWN]
    || currentDpad[DPAD_LEFT]
    || currentDpad[DPAD_RIGHT]
    || currentButtons != 0
    || isAnalogPressed(currentLeftAnalog[0])
    || isAnalogPressed(currentLeftAnalog[1])
    || currentLeftAnalog[2] > (MIN_ANALOG_VALUE + ANALOG_PRESSED_TOL)
    || isAnalogPressed(currentRightAnalog[0])
    || isAnalogPressed(currentRightAnalog[1])
    || currentRightAnalog[2] > (MIN_ANALOG_VALUE + ANALOG_PRESSED_TOL));
}

//--------------------------------------------------------------------+
// EXTERNAL API
//--------------------------------------------------------------------+
void UsbGamepad::setAnalogThumbX(bool isLeft, int8_t x)
{
  x = limit_value(x, MIN_ANALOG_VALUE, MAX_ANALOG_VALUE);
  int8_t lastX = 0;
  if (isLeft)
  {
    lastX = currentLeftAnalog[0];
    currentLeftAnalog[0] = x;
  }
  else
  {
    lastX = currentRightAnalog[0];
    currentRightAnalog[0] = x;
  }
  buttonsUpdated = buttonsUpdated || (x != lastX);
}

void UsbGamepad::setAnalogThumbY(bool isLeft, int8_t y)
{
  y = limit_value(y, MIN_ANALOG_VALUE, MAX_ANALOG_VALUE);
  int8_t lastY = 0;
  if (isLeft)
  {
    lastY = currentLeftAnalog[1];
    currentLeftAnalog[1] = y;
  }
  else
  {
    lastY = currentRightAnalog[1];
    currentRightAnalog[1] = y;
  }
  buttonsUpdated = buttonsUpdated || (y != lastY);
}

void UsbGamepad::setAnalogTrigger(bool isLeft, int8_t z)
{
  z = limit_value(z, MIN_ANALOG_VALUE, MAX_ANALOG_VALUE);
  int8_t lastZ = 0;
  if (isLeft)
  {
    lastZ = currentLeftAnalog[2];
    currentLeftAnalog[2] = z;
  }
  else
  {
    lastZ = currentRightAnalog[2];
    currentRightAnalog[2] = z;
  }
  buttonsUpdated = buttonsUpdated || (z != lastZ);
}

int8_t UsbGamepad::getAnalogThumbX(bool isLeft)
{
  if (isLeft)
  {
    return currentLeftAnalog[0];
  }
  else
  {
    return currentRightAnalog[0];
  }
}

int8_t UsbGamepad::getAnalogThumbY(bool isLeft)
{
  if (isLeft)
  {
    return currentLeftAnalog[1];
  }
  else
  {
    return currentRightAnalog[1];
  }
}

int8_t UsbGamepad::getAnalogTrigger(bool isLeft)
{
  if (isLeft)
  {
    return currentLeftAnalog[2];
  }
  else
  {
    return currentRightAnalog[2];
  }
}

void UsbGamepad::setDigitalPad(UsbGamepad::DpadButtons button, bool isPressed)
{
  bool oldValue = currentDpad[button];
  currentDpad[button] = isPressed;
  buttonsUpdated = buttonsUpdated || (oldValue != currentDpad[button]);
}

void UsbGamepad::setButtonMask(uint32_t mask, bool isPressed)
{
  uint32_t lastButtons = currentButtons;
  if (isPressed)
  {
    currentButtons |= mask;
  }
  else
  {
    currentButtons &= ~mask;
  }
  buttonsUpdated = buttonsUpdated || (lastButtons != currentButtons);
}

void UsbGamepad::setButton(uint8_t button, bool isPressed)
{
  setButtonMask(1 << button, isPressed);
}

void UsbGamepad::updateAllReleased()
{
  if (isButtonPressed())
  {
    currentLeftAnalog[0] = 0;
    currentLeftAnalog[1] = 0;
    currentLeftAnalog[2] = MIN_ANALOG_VALUE;
    currentRightAnalog[0] = 0;
    currentRightAnalog[1] = 0;
    currentRightAnalog[2] = MIN_ANALOG_VALUE;
    currentDpad[DPAD_UP] = false;
    currentDpad[DPAD_DOWN] = false;
    currentDpad[DPAD_LEFT] = false;
    currentDpad[DPAD_RIGHT] = false;
    currentButtons = 0;
    buttonsUpdated = true;
  }
}

uint8_t UsbGamepad::getHatValue()
{
  if (currentDpad[DPAD_UP])
  {
    if (currentDpad[DPAD_LEFT])
    {
      return GAMEPAD_HAT_UP_LEFT;
    }
    else if (currentDpad[DPAD_RIGHT])
    {
      return GAMEPAD_HAT_UP_RIGHT;
    }
    else
    {
      return GAMEPAD_HAT_UP;
    }
  }
  else if (currentDpad[DPAD_DOWN])
  {
    if (currentDpad[DPAD_LEFT])
    {
      return GAMEPAD_HAT_DOWN_LEFT;
    }
    else if (currentDpad[DPAD_RIGHT])
    {
      return GAMEPAD_HAT_DOWN_RIGHT;
    }
    else
    {
      return GAMEPAD_HAT_DOWN;
    }
  }
  else if (currentDpad[DPAD_LEFT])
  {
    return GAMEPAD_HAT_LEFT;
  }
  else if (currentDpad[DPAD_RIGHT])
  {
    return GAMEPAD_HAT_RIGHT;
  }
  else
  {
    return GAMEPAD_HAT_CENTERED;
  }
}

bool UsbGamepad::send(bool force)
{
  if (buttonsUpdated || force)
  {
    bool sent = sendReport(interfaceId, reportId);
    if (sent)
    {
      buttonsUpdated = false;
    }
    return sent;
  }
  else
  {
    return true;
  }
}

uint8_t UsbGamepad::getReportSize()
{
  return sizeof(hid_gamepad_report_t);
}

uint16_t UsbGamepad::getReport(uint8_t *buffer, uint16_t reqlen)
{
  // Build the report
  hid_gamepad_report_t report;
  report.x = currentLeftAnalog[0];
  report.y = currentLeftAnalog[1];
  report.z = currentLeftAnalog[2];
  report.rz = currentRightAnalog[2];
  report.rx = currentRightAnalog[0];
  report.ry = currentRightAnalog[1];
  report.hat = getHatValue();
  report.buttons = currentButtons;
  // Copy report into buffer
  uint16_t setLen = (sizeof(report) <= reqlen) ? sizeof(report) : reqlen;
  memcpy(buffer, &report, setLen);
  return setLen;
}
