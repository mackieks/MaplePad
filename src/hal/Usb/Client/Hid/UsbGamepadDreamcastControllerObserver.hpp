#ifndef __USB_CONTROLLER_DREAMCAST_CONTROLLER_OBSERVER_H__
#define __USB_CONTROLLER_DREAMCAST_CONTROLLER_OBSERVER_H__

#include "hal/Usb/DreamcastControllerObserver.hpp"
#include "UsbGamepad.h"

//! Yes, I know this name is ridiculous, but at least it's descriptive!
//! This connects the Dreamcast controller observer to a USB gamepad device
class UsbGamepadDreamcastControllerObserver : public DreamcastControllerObserver
{
    public:
        //! Constructor for UsbKeyboardGenesisControllerObserver
        //! @param[in] usbController  The USB controller to update when keys are pressed or released
        UsbGamepadDreamcastControllerObserver(UsbGamepad& usbController);

        //! Sets the current Dreamcast controller condition
        //! @param[in] controllerCondition  The current condition of the Dreamcast controller
        virtual void setControllerCondition(const ControllerCondition& controllerCondition) final;

        //! Sets the current Dreamcast secondary controller condition
        //! @param[in] secondaryControllerCondition  The current secondary condition of the
        //!                                          Dreamcast controller
        virtual void setSecondaryControllerCondition(
            const SecondaryControllerCondition& secondaryControllerCondition) final;

        //! Called when controller connected
        virtual void controllerConnected() final;

        //! Called when controller disconnected
        virtual void controllerDisconnected() final;

    private:
        //! The USB controller I update
        UsbGamepad& mUsbController;
};

#endif // __USB_CONTROLLER_DREAMCAST_CONTROLLER_OBSERVER_H__
