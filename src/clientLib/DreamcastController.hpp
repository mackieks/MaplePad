#pragma once

#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"
#include "dreamcast_structures.h"
#include "GamepadHost.hpp"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico_configurations.h"

namespace client
{
class DreamcastController : public DreamcastPeripheralFunction
{
public:
    struct EnabledControls
    {
        bool enLeftD;
        bool enRightD;
        bool enLeftA;
        bool enRightA;
        bool enL;
        bool enR;
        bool enStart;
        bool enA;
        bool enB;
        bool enC;
        bool enD;
        bool enX;
        bool enY;
        bool enZ;
    };

public:
    //! Default constructor (1) - Sets up controller with default controls enabled
    DreamcastController();

    //! Constructor (2)
    //! @param[in] enabledControls  The controls to tell the host that are enabled
    DreamcastController(EnabledControls enabledControls);

    //! Sets what controls are enabled
    //! @note After this is called and if the peripheral that this function belongs to is connected,
    //!       it must be forcibly disconnected then reconnected in order to get the host to
    //!       re-request this information.
    //! @param[in] enabledControls  The controls to tell the host that are enabled
    void setEnabledControls(EnabledControls enabledControls);

    //! Inherited from DreamcastPeripheralFunction
    virtual bool handlePacket(const MaplePacket& in, MaplePacket& out) final;

    //! Inherited from DreamcastPeripheralFunction
    virtual void reset() final;

    //! Inherited from DreamcastPeripheralFunction
    virtual uint32_t getFunctionDefinition() final;

    //! Sets the raw controller condition
    void setCondition(controller_condition_t condition);

    //! @returns the number of condition samples made
    uint32_t getConditionSamples();

    bool triggerMenu();

    bool dpadDownPressed();

    bool dpadUpPressed();

private:
    //! Updates mConditionAndMask and mConditionOrMask based on mEnabledControls
    void updateConditionMasks();

    //! Sets the standard set of gamepad controls to the controller condition
    void setControls();

    //! @returns the value constrained by the params
    uint8_t map(uint8_t x, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max);

    //! Reads and returns the selected ADC input for the trigger
    //! @returns the value read from ADC constrained by the params
    uint8_t getTriggerADCInput(uint8_t adcSelect, uint8_t min, uint8_t max, uint8_t deadzone, uint8_t antiDeadzone, bool invert);

    //! Reads and returns the selected ADC input for the joystick
    //! @returns the value read from ADC constrained by the params
    uint8_t getJoystickADCInput(uint8_t adcSelect, uint8_t min, uint8_t max, uint8_t deadzone, uint8_t antiDeadzone, uint8_t center, bool invert);

    
  private:
    //! All controls reported as enabled to the host
    EnabledControls mEnabledControls;
    //! AND mask to apply to newly set condition
    uint32_t mConditionAndMask[2];
    //! OR mas to apply to newly set condition
    uint32_t mConditionOrMask[2];
    //! Last set controller condition
    uint32_t mCondition[2];
    //! Number of condition samples requested by host
    uint32_t mConditionSamples;
};
}