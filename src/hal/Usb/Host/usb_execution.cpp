#include "hal/Usb/host_usb_interface.hpp"

#include "hid_app.hpp"

#include "tusb_config.h"
#include "bsp/board.h"
#include "tusb.h"

void usb_init()
{
    board_init();
    tusb_init();
}

void usb_task(uint64_t timeUs)
{
    tuh_task();
    hid_app_task(timeUs);
}
