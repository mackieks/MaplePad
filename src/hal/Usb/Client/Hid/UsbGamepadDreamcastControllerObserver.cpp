#include "UsbGamepadDreamcastControllerObserver.hpp"

UsbGamepadDreamcastControllerObserver::UsbGamepadDreamcastControllerObserver(UsbGamepad& usbController) :
    mUsbController(usbController)
{}

void UsbGamepadDreamcastControllerObserver::setControllerCondition(const ControllerCondition& controllerCondition)
{
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_A, 0 == controllerCondition.a);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_B, 0 == controllerCondition.b);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_C, 0 == controllerCondition.c);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_X, 0 == controllerCondition.x);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_Y, 0 == controllerCondition.y);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_Z, 0 == controllerCondition.z);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_START, 0 == controllerCondition.start);

    // Mapping these to random unique buttons just in case something out there uses them
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_TL, 0 == controllerCondition.rightb);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_TR, 0 == controllerCondition.leftb);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_TL2, 0 == controllerCondition.downb);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_TR2, 0 == controllerCondition.upb);
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_SELECT, 0 == controllerCondition.d);

    mUsbController.setDigitalPad(UsbGamepad::DPAD_UP, 0 == controllerCondition.up);
    mUsbController.setDigitalPad(UsbGamepad::DPAD_DOWN, 0 == controllerCondition.down);
    mUsbController.setDigitalPad(UsbGamepad::DPAD_LEFT, 0 == controllerCondition.left);
    mUsbController.setDigitalPad(UsbGamepad::DPAD_RIGHT, 0 == controllerCondition.right);

    mUsbController.setAnalogTrigger(true, static_cast<int32_t>(controllerCondition.l) - 128);
    mUsbController.setAnalogTrigger(false, static_cast<int32_t>(controllerCondition.r) - 128);

    mUsbController.setAnalogThumbX(true, static_cast<int32_t>(controllerCondition.lAnalogLR) - 128);
    mUsbController.setAnalogThumbY(true, static_cast<int32_t>(controllerCondition.lAnalogUD) - 128);
    mUsbController.setAnalogThumbX(false, static_cast<int32_t>(controllerCondition.rAnalogLR) - 128);
    mUsbController.setAnalogThumbY(false, static_cast<int32_t>(controllerCondition.rAnalogUD) - 128);

    mUsbController.send();
}

void UsbGamepadDreamcastControllerObserver::setSecondaryControllerCondition(
    const SecondaryControllerCondition& secondaryControllerCondition)
{
    mUsbController.setButton(UsbGamepad::GAMEPAD_BUTTON_MODE, 0 == secondaryControllerCondition.a);
    mUsbController.setButton(UsbGamepad::BUTTON15, 0 == secondaryControllerCondition.b);
    mUsbController.setButton(UsbGamepad::BUTTON16, 0 == secondaryControllerCondition.up);
    mUsbController.setButton(UsbGamepad::BUTTON17, 0 == secondaryControllerCondition.down);
    mUsbController.setButton(UsbGamepad::BUTTON18, 0 == secondaryControllerCondition.left);
    mUsbController.setButton(UsbGamepad::BUTTON19, 0 == secondaryControllerCondition.right);

    mUsbController.send();
}

void UsbGamepadDreamcastControllerObserver::controllerConnected()
{
    mUsbController.updateControllerConnected(true);
    mUsbController.send(true);
}

void UsbGamepadDreamcastControllerObserver::controllerDisconnected()
{
    mUsbController.updateControllerConnected(false);
    mUsbController.send(true);
}
