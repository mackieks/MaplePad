#ifndef __USB_INTERFACE_H__
#define __USB_INTERFACE_H__

#include "UsbFileSystem.hpp"
#include "DreamcastControllerObserver.hpp"
#include "hal/System/MutexInterface.hpp"
#include <vector>

//! @returns array of the USB controller observers
DreamcastControllerObserver** get_usb_controller_observers();
//! USB initialization
void usb_init(
  MutexInterface* mscMutex,
  MutexInterface* cdcStdioMutex);
//! USB task that needs to be called constantly by main()
void usb_task();
//! @returns number of USB controllers
uint32_t get_num_usb_controllers();

//! Must return the file system
UsbFileSystem& usb_msc_get_file_system();


#endif // __USB_INTERFACE_H__
