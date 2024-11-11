#include "UsbControllerDevice.h"
#include <stdint.h>
#include "class/hid/hid_device.h"

UsbControllerDevice::UsbControllerDevice() : mIsUsbConnected(false), mIsControllerConnected(false) {}
UsbControllerDevice::~UsbControllerDevice() {}

void UsbControllerDevice::updateUsbConnected(bool connected)
{
  mIsUsbConnected = connected;
}

bool UsbControllerDevice::isUsbConnected()
{
  return mIsUsbConnected;
}

void UsbControllerDevice::updateControllerConnected(bool connected)
{
  if (connected != mIsControllerConnected)
  {
    updateAllReleased();
  }
  mIsControllerConnected = connected;
}

bool UsbControllerDevice::isControllerConnected()
{
  return mIsControllerConnected;
}

bool UsbControllerDevice::sendReport(uint8_t instance, uint8_t report_id)
{
  bool sent = false;
  if (isUsbConnected())
  {
    uint8_t reportSize = getReportSize();
    uint8_t reportBuffer[reportSize];
    getReport(reportBuffer, reportSize);
    sent = tud_hid_n_report(instance, report_id, reportBuffer, reportSize);
  }
  return sent;
}
