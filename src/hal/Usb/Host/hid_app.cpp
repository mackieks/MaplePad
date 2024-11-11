// This file was modified from example code in tinyusb library
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "hid_app.hpp"

#include "bsp/board.h"
#include "tusb.h"
#include "tusb_config.h"

#include "GamepadHost.hpp"
#include <string.h>

#include "VibrationObserver.hpp"

static GamepadHost* pGamepadHost = nullptr;

typedef struct TU_ATTR_PACKED {
  // First 16 bits set what data is pertinent in this structure (1 = set; 0 = not set)
  uint8_t set_rumble : 1;
  uint8_t set_led : 1;
  uint8_t set_led_blink : 1;
  uint8_t set_ext_write : 1;
  uint8_t set_left_volume : 1;
  uint8_t set_right_volume : 1;
  uint8_t set_mic_volume : 1;
  uint8_t set_speaker_volume : 1;
  uint8_t set_flags2;

  uint8_t reserved;

  uint8_t motor_right;
  uint8_t motor_left;

  uint8_t lightbar_red;
  uint8_t lightbar_green;
  uint8_t lightbar_blue;
  uint8_t lightbar_blink_on;
  uint8_t lightbar_blink_off;

  uint8_t ext_data[8];

  uint8_t volume_left;
  uint8_t volume_right;
  uint8_t volume_mic;
  uint8_t volume_speaker;

  uint8_t other[9];
} sony_ds4_output_report_t;

class Ds4VibrationClient : public VibrationObserver
{
public:
  inline Ds4VibrationClient() :
    mIsMounted(false),
    mIsStopped(true),
    mDevAddr(0),
    mInstance(0),
    mCurrentTime(0),
    mStopTime(0),
    mStartingIntensity(0),
    mCurrentInclination(0),
    mCurrentMotorIntensity(0),
    mLastUpdateTime(0)
  {}

  inline void mount(uint8_t dev_addr, uint8_t instance)
  {
    mDevAddr = dev_addr;
    mInstance = instance;
    mIsMounted = true;
  }

  inline void unmount()
  {
    mIsMounted = false;
  }

  inline bool isMounted()
  {
    return mIsMounted;
  }

  inline bool isDevice(uint8_t dev_addr, uint8_t instance)
  {
    return (mIsMounted && dev_addr == mDevAddr && instance == mInstance);
  }

  inline void task(uint64_t time)
  {
    mCurrentTime = time;
    if (mIsMounted)
    {
      if (!mIsStopped)
      {
        if (time >= mStopTime)
        {
          mIsStopped = true;
          mCurrentMotorIntensity = 0;
        }
        else if (mCurrentInclination < 0)
        {
          uint32_t remainingDuration = mStopTime - time;
          float mult = (float)remainingDuration / mDuration;
          mCurrentMotorIntensity = mStartingIntensity * mult + 0.5;
        }
        else if (mCurrentInclination > 0)
        {
          uint32_t remainingDuration = mStopTime - time;
          float mult = (float)remainingDuration / mDuration;
          uint8_t delta = 255 - mStartingIntensity;
          mCurrentMotorIntensity = 255 - (delta * mult + 0.5);
        }
      }

      // Update motor every 50 ms
      if (time - mLastUpdateTime > 50000)
      {
        sendVibration(mCurrentMotorIntensity);
        mLastUpdateTime = time;
      }
    }
  }

  inline void sendVibration(uint8_t intensity)
  {
    sony_ds4_output_report_t output_report = {0};
    output_report.set_rumble = 1;
    output_report.motor_left = intensity;
    output_report.motor_right = intensity;
    tuh_hid_send_report(mDevAddr, mInstance, 5, &output_report, sizeof(output_report));
  }

  virtual inline void vibrate(
    float frequency, float intensity, int8_t inclination, float duration) final
  {
    if (mIsMounted)
    {
      mDuration = (duration * 1000000);
      mStopTime = mCurrentTime + mDuration;
      mCurrentInclination = inclination;
      mStartingIntensity = intensity * 255;
      mIsStopped = false;
      mCurrentMotorIntensity = mStartingIntensity;
      mLastUpdateTime = mCurrentTime;
      sendVibration(mCurrentMotorIntensity);
    }
  }

private:
  bool mIsMounted;
  bool mIsStopped;
  uint8_t mDevAddr;
  uint8_t mInstance;
  uint64_t mCurrentTime;
  uint64_t mStopTime;
  uint32_t mDuration;
  uint8_t mStartingIntensity;
  int8_t mCurrentInclination;

  uint8_t mCurrentMotorIntensity;
  uint64_t mLastUpdateTime;
};

Ds4VibrationClient ds4_vibration_client;

// Sony DS4 report layout detail https://www.psdevwiki.com/ps4/DS4-USB
typedef struct TU_ATTR_PACKED
{
  uint8_t x, y, z, rz; // joystick

  struct {
    uint8_t dpad     : 4; // (hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
    uint8_t square   : 1; // west
    uint8_t cross    : 1; // south
    uint8_t circle   : 1; // east
    uint8_t triangle : 1; // north
  };

  struct {
    uint8_t l1     : 1;
    uint8_t r1     : 1;
    uint8_t l2     : 1;
    uint8_t r2     : 1;
    uint8_t share  : 1;
    uint8_t option : 1;
    uint8_t l3     : 1;
    uint8_t r3     : 1;
  };

  struct {
    uint8_t ps      : 1; // playstation button
    uint8_t tpad    : 1; // track pad click
    uint8_t counter : 6; // +1 each report
  };

  // comment out since not used by this example
  uint8_t l2_trigger; // 0 released, 0xff fully pressed
  uint8_t r2_trigger; // as above

  //  uint16_t timestamp;
  //  uint8_t  battery;
  //
  //  int16_t gyro[3];  // x, y, z;
  //  int16_t accel[3]; // x, y, z

  // there is still lots more info

} sony_ds4_report_t;

VibrationObserver* get_usb_vibration_observer()
{
  return &ds4_vibration_client;
}

void set_gamepad_host(GamepadHost* ctrlr)
{
  pGamepadHost = ctrlr;
}

// check if device is Sony DualShock 4
static inline bool is_sony_ds4(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4)) // Sony DualShock4
           || (vid == 0x0f0d && pid == 0x005e)                 // Hori FC4
           || (vid == 0x0f0d && pid == 0x00ee)                 // Hori PS4 Mini (PS4-099U)
           || (vid == 0x1f4f && pid == 0x1002)                 // ASW GG xrd controller
         );
}

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

void hid_app_task(uint64_t timeUs)
{
  ds4_vibration_client.task(timeUs);
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  (void)desc_report;
  (void)desc_len;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  // Sony DualShock 4 [CUH-ZCT2x]
  if ( is_sony_ds4(dev_addr) )
  {
    ds4_vibration_client.mount(dev_addr, instance);
    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      printf("Error: cannot request to receive report\r\n");
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  if (ds4_vibration_client.isDevice(dev_addr, instance))
  {
    ds4_vibration_client.unmount();
  }
}

void process_sony_ds4(uint8_t const* report, uint16_t len)
{
  //const char* dpad_str[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "none" };

  // previous report used to compare for changes
  static sony_ds4_report_t prev_report = { 0 };

  uint8_t const report_id = report[0];
  report++;
  len--;

  // all buttons state is stored in ID 1
  if (report_id == 1)
  {
    sony_ds4_report_t ds4_report;
    memcpy(&ds4_report, report, sizeof(ds4_report));

    // counter is +1, assign to make it easier to compare 2 report
    prev_report.counter = ds4_report.counter;

    if (pGamepadHost != nullptr)
    {
      GamepadHost::Controls condition = {};
      condition.l2 = ds4_report.l2_trigger;
      condition.r2 = ds4_report.r2_trigger;
      condition.south = ds4_report.cross;
      condition.east = ds4_report.circle;
      condition.west = ds4_report.square;
      condition.north = ds4_report.triangle;
      condition.r1 = ds4_report.r1;
      condition.l1 = ds4_report.l1;
      condition.l3 = ds4_report.l3;
      // Both PS and touchpad press will register as start
      condition.start = ds4_report.ps || ds4_report.tpad;
      condition.menu = ds4_report.option;
      switch(ds4_report.dpad)
      {
        case 0:
          condition.hat = GamepadHost::Hat::UP;
          break;
        case 1:
          condition.hat = GamepadHost::Hat::UP_RIGHT;
          break;
        case 2:
          condition.hat = GamepadHost::Hat::RIGHT;
          break;
        case 3:
          condition.hat = GamepadHost::Hat::DOWN_RIGHT;
          break;
        case 4:
          condition.hat = GamepadHost::Hat::DOWN;
          break;
        case 5:
          condition.hat = GamepadHost::Hat::DOWN_LEFT;
          break;
        case 6:
          condition.hat = GamepadHost::Hat::LEFT;
          break;
        case 7:
          condition.hat = GamepadHost::Hat::UP_LEFT;
          break;
        default:
          condition.hat = GamepadHost::Hat::NEUTRAL;
          break;
      }
      condition.ry = ds4_report.rz;
      condition.rx = ds4_report.z;
      condition.ly = ds4_report.y;
      condition.lx = ds4_report.x;

      pGamepadHost->setControls(condition);
    }

    prev_report = ds4_report;
  }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if ( is_sony_ds4(dev_addr) )
  {
    process_sony_ds4(report, len);
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}