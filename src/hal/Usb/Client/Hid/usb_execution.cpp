#include "UsbControllerDevice.h"
#include "UsbGamepadDreamcastControllerObserver.hpp"
#include "UsbGamepad.h"
#include "configuration.h"
#include "hal/Usb/client_usb_interface.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "device/dcd.h"
#include "usb_descriptors.h"
#include "class/hid/hid_device.h"
#include "msc_disk.hpp"
#include "cdc.hpp"

UsbGamepad usbGamepads[NUMBER_OF_GAMEPADS] = {
  UsbGamepad(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 0)),
  UsbGamepad(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 1)),
  UsbGamepad(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 2)),
  UsbGamepad(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 3))
};

UsbGamepadDreamcastControllerObserver usbGamepadDreamcastControllerObservers[NUMBER_OF_GAMEPADS] = {
  UsbGamepadDreamcastControllerObserver(usbGamepads[0]),
  UsbGamepadDreamcastControllerObserver(usbGamepads[1]),
  UsbGamepadDreamcastControllerObserver(usbGamepads[2]),
  UsbGamepadDreamcastControllerObserver(usbGamepads[3])
};

UsbControllerDevice* devices[NUMBER_OF_GAMEPADS] = {
  &usbGamepads[0],
  &usbGamepads[1],
  &usbGamepads[2],
  &usbGamepads[3]
};

DreamcastControllerObserver* observers[NUMBER_OF_GAMEPADS] = {
  &usbGamepadDreamcastControllerObservers[0],
  &usbGamepadDreamcastControllerObservers[1],
  &usbGamepadDreamcastControllerObservers[2],
  &usbGamepadDreamcastControllerObservers[3]
};

DreamcastControllerObserver** get_usb_controller_observers()
{
  return observers;
}

uint32_t get_num_usb_controllers()
{
  uint8_t installedGamepads = get_usb_descriptor_number_of_gamepads();

  if (installedGamepads <= NUMBER_OF_GAMEPADS)
  {
    return installedGamepads;
  }
  else
  {
    return NUMBER_OF_GAMEPADS;
  }
}

bool usbEnabled = false;

UsbControllerDevice** pAllUsbDevices = nullptr;

uint8_t numUsbDevices = 0;

bool usbDisconnecting = false;
absolute_time_t usbDisconnectTime;

void set_usb_devices(UsbControllerDevice** devices, uint8_t n)
{
  pAllUsbDevices = devices;
  numUsbDevices = n;
}

bool gIsConnected = false;

void led_task()
{
#if USB_LED_PIN >= 0
  static bool ledOn = false;
  static uint32_t startMs = 0;
  if (usbDisconnecting)
  {
    // Currently counting down to disconnect; flash LED
    static const uint32_t BLINK_TIME_MS = 500;
    uint32_t t = board_millis() - startMs;
    if (t >= BLINK_TIME_MS)
    {
      startMs += BLINK_TIME_MS;
      ledOn = !ledOn;
    }
  }
  else
  {
    bool keyPressed = false;
    UsbControllerDevice** pdevs = pAllUsbDevices;
    for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
    {
      if ((*pdevs)->isButtonPressed())
      {
        keyPressed = true;
        break;
      }
    }
    if (gIsConnected)
    {
      // When connected, LED is ON only when no key is pressed
      ledOn = !keyPressed;
    }
    else
    {
      // When not connected, LED blinks when key is pressed
      static const uint32_t BLINK_TIME_MS = 100;
      uint32_t t = board_millis() - startMs;
      if (t >= BLINK_TIME_MS)
      {
        startMs += BLINK_TIME_MS;
        ledOn = !ledOn;
      }
      ledOn = ledOn && keyPressed;
    }
  }
  gpio_put(USB_LED_PIN, ledOn);
#endif

#if SIMPLE_USB_LED_PIN >= 0
  gpio_put(SIMPLE_USB_LED_PIN, gIsConnected);
#endif

}

void usb_init(
  MutexInterface* mscMutex,
  MutexInterface* cdcStdioMutex)
{
  uint32_t numDevices = get_num_usb_controllers();

  for (uint32_t i = 0; i < numDevices; ++i)
  {
    usbGamepads[i].setInterfaceId(ITF_NUM_GAMEPAD(numDevices, i));
  }

  uint32_t max = sizeof(devices) / sizeof(devices[1]);
  if (numDevices > max)
  {
    numDevices = max;
  }
  set_usb_devices(devices, numDevices);
  
  board_init();
  tusb_init();
  msc_init(mscMutex);
  cdc_init(cdcStdioMutex);

#if USB_LED_PIN >= 0
  gpio_init(USB_LED_PIN);
  gpio_set_dir_out_masked(1<<USB_LED_PIN);
#endif

#if SIMPLE_USB_LED_PIN >= 0
  gpio_init(SIMPLE_USB_LED_PIN);
  gpio_set_dir_out_masked(1<<SIMPLE_USB_LED_PIN);
#endif
}

void usb_task()
{
  tud_task(); // tinyusb device task
  led_task();
  cdc_task();
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  UsbControllerDevice** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(true);
    (*pdevs)->send(true);
  }
  gIsConnected = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  UsbControllerDevice** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(false);
  }
  gIsConnected = false;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  UsbControllerDevice** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(false);
  }
  gIsConnected = false;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  UsbControllerDevice** pdevs = pAllUsbDevices;
  for (uint32_t i = numUsbDevices; i > 0; --i, ++pdevs)
  {
    (*pdevs)->updateUsbConnected(true);
    (*pdevs)->send(true);
  }
  gIsConnected = true;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void) report_id;
  (void) report_type;
  if (instance >= numUsbDevices)
  {
    return 0;
  }
  else
  {
    // Build the report for the given report ID and return the size set
    return pAllUsbDevices[instance]->getReport(buffer, reqlen);
  }
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
  (void) instance;
  (void) report_type;

  // echo back anything we received from host
  tud_hid_report(report_id, buffer, bufsize);
}