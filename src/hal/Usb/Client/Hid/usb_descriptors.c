/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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

#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid_device.h"
#include "pico/unique_id.h"
#include "configuration.h"
#include <string.h>

static uint8_t numberOfGamepads = NUMBER_OF_GAMEPADS;

void set_usb_descriptor_number_of_gamepads(uint8_t num)
{
    if (num > NUMBER_OF_GAMEPADS)
    {
        num = NUMBER_OF_GAMEPADS;
    }
    numberOfGamepads = num;
}

uint8_t get_usb_descriptor_number_of_gamepads()
{
    return numberOfGamepads;
}

#undef TUD_HID_REPORT_DESC_GAMEPAD

// Tweak the gamepad descriptor so that the minimum value on analog controls is -128 instead of -127
#define TUD_HID_REPORT_DESC_GAMEPAD(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min -127, max 127 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN    ( 0x80                                   ) ,\
    HID_LOGICAL_MAX    ( 0x7f                                   ) ,\
    HID_REPORT_COUNT   ( 6                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN    ( 1                                      ) ,\
    HID_LOGICAL_MAX    ( 8                                      ) ,\
    HID_PHYSICAL_MIN   ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                                 ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 32 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 32                                     ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 32                                     ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    // VID 1209 comes from https://pid.codes/
    // PID 2F07 is a subassignment granted through github
    // https://github.com/pidcodes/pidcodes.github.com/blob/74f95539d2ad737c1ba2871eeb25b3f5f5d5c790/1209/2F07/index.md
    .idVendor           = 0x1209,
    .idProduct          = 0x2F07,

    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_GAMEPAD()
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    for (uint8_t i = 0; i < numberOfGamepads; ++i)
    {
        if (instance == ITF_NUM_GAMEPAD(numberOfGamepads, i))
        {
            return desc_hid_report;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#if USB_CDC_ENABLED
    #define DEBUG_CONFIG_LEN TUD_CDC_DESC_LEN
#else
    #define DEBUG_CONFIG_LEN 0
#endif

#define GET_CONFIG_LEN(numGamepads) (TUD_CONFIG_DESC_LEN + (numGamepads * TUD_HID_DESC_LEN) + DEBUG_CONFIG_LEN + TUD_MSC_DESC_LEN)

// Endpoint definitions (must all be unique)
#define EPIN_GAMEPAD1   (0x84)
#define EPIN_GAMEPAD2   (0x83)
#define EPIN_GAMEPAD3   (0x82)
#define EPIN_GAMEPAD4   (0x81)
#define EPOUT_MSC       (0x05)
#define EPIN_MSC        (0x85)
#define EPIN_CDC_NOTIF  (0x86)
#define EPOUT_CDC       (0x07)
#define EPIN_CDC        (0x87)

#define PLAYER_TO_STR_IDX(player) (player + 4)

uint8_t player_to_epin(uint8_t player)
{
    switch (player)
    {
        case 0: return EPIN_GAMEPAD1;
        case 1: return EPIN_GAMEPAD2;
        case 2: return EPIN_GAMEPAD3;
        default:
        case 3: return EPIN_GAMEPAD4;
    }
}

#define GAMEPAD_REPORT_SIZE (1 + sizeof(hid_gamepad_report_t))

#define CONFIG_HEADER(numGamepads) \
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT(numGamepads), 0, GET_CONFIG_LEN(numGamepads), TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 400)

#define GAMEPAD_CONFIG_DESC(itfNum, strIdx, endpt) \
    TUD_HID_DESCRIPTOR(itfNum, strIdx, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), endpt, GAMEPAD_REPORT_SIZE, 1)

// Only doing transfer at full speed since each file will only be about 128KB, max of 8 files
#define MSC_DESCRIPTOR(numGamepads) TUD_MSC_DESCRIPTOR(ITF_NUM_MSC(numGamepads), 8, EPOUT_MSC, EPIN_MSC, 64)

#define CDC_DESCRIPTOR(numGamepads) TUD_CDC_DESCRIPTOR(ITF_NUM_CDC(numGamepads), 9, EPIN_CDC_NOTIF, 8, EPOUT_CDC, EPIN_CDC, 64)

uint8_t desc_configuration[] =
{
    CONFIG_HEADER(NUMBER_OF_GAMEPADS),

    // *************************************************************************
    // * Gamepad Descriptors                                                   *
    // *************************************************************************

    GAMEPAD_CONFIG_DESC(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 3), PLAYER_TO_STR_IDX(3), EPIN_GAMEPAD4),
    GAMEPAD_CONFIG_DESC(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 2), PLAYER_TO_STR_IDX(2), EPIN_GAMEPAD3),
    GAMEPAD_CONFIG_DESC(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 1), PLAYER_TO_STR_IDX(1), EPIN_GAMEPAD2),
    GAMEPAD_CONFIG_DESC(ITF_NUM_GAMEPAD(NUMBER_OF_GAMEPADS, 0), PLAYER_TO_STR_IDX(0), EPIN_GAMEPAD1),

    // *************************************************************************
    // * Storage Device Descriptor                                             *
    // *************************************************************************

    MSC_DESCRIPTOR(NUMBER_OF_GAMEPADS),

    // *************************************************************************
    // * Communication Device Descriptor  (for debug messaging)                *
    // *************************************************************************

#if USB_CDC_ENABLED
    CDC_DESCRIPTOR(NUMBER_OF_GAMEPADS),
#endif
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations

    // Build the config based on number of players
    uint32_t offset = 0;

    uint8_t header[] = {
        CONFIG_HEADER(numberOfGamepads)
    };
    memcpy(&desc_configuration[offset], header, sizeof(header));
    offset += sizeof(header);

    for (uint8_t i = 0; i < numberOfGamepads; ++i)
    {
        uint8_t idx = numberOfGamepads - i - 1;
        uint8_t gpConfig[] = {
            GAMEPAD_CONFIG_DESC(ITF_NUM_GAMEPAD(numberOfGamepads, idx), PLAYER_TO_STR_IDX(idx), player_to_epin(idx))
        };
        memcpy(&desc_configuration[offset], gpConfig, sizeof(gpConfig));
        offset += sizeof(gpConfig);
    }

    uint8_t mscConfig[] = {
        MSC_DESCRIPTOR(numberOfGamepads)
    };
    memcpy(&desc_configuration[offset], mscConfig, sizeof(mscConfig));
    offset += sizeof(mscConfig);

#if USB_CDC_ENABLED
    uint8_t cdcConfig[] = {
        CDC_DESCRIPTOR(numberOfGamepads)
    };
    memcpy(&desc_configuration[offset], cdcConfig, sizeof(cdcConfig));
    offset += sizeof(cdcConfig);
#endif

    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] =
{
    (const char[]) {0x09, 0x04}, // 0: is supported language is English (0x0409)
    "OrangeFox86",               // 1: Manufacturer
    "Dreamcast Controller USB",  // 2: Product
    NULL,                        // 3: Serial (special case; get pico serial)
    "P1",                        // 4: Gamepad 1
    "P2",                        // 5: Gamepad 2
    "P3",                        // 6: Gamepad 3
    "P4",                        // 7: Gamepad 4
    "MSC",                       // 8: Mass Storage Class
    "CDC",                       // 9: Communication Device Class
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;

    uint8_t chr_count;
    char buffer[32] = {0};

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        // Convert ASCII string into UTF-16

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) return NULL;

        const char *str = string_desc_arr[index];

        if (index == PLAYER_TO_STR_IDX(0) && numberOfGamepads == 1)
        {
            // Special case - if there is only 1 controller, change the label
            str = "Dreamcast Controller";
        }
        else if (str == NULL)
        {
            if (index == 3)
            {
                // Special case: try to get pico serial number
                pico_get_unique_board_id_string(buffer, sizeof(buffer));
                if (buffer[0] != '\0')
                {
                    str = buffer;
                }
                else
                {
                    // Something failed, have host assign serial
                    return NULL;
                }
            }
            else
            {
                return NULL;
            }
        }

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
