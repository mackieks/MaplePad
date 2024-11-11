#pragma once

#include "DreamcastPeripheralFunction.hpp"
#include "dreamcast_constants.h"
#include "dreamcast_structures.h"
#include "GamepadHost.hpp"

namespace client
{
class DreamcastController : public DreamcastPeripheralFunction, public GamepadHost
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

    //! Sets the standard set of gamepad controls to the controller condition
    virtual void setControls(const Controls& controls) final;

    //! @returns the number of condition samples made
    uint32_t getConditionSamples();

private:
    //! Updates mConditionAndMask and mConditionOrMask based on mEnabledControls
    void updateConditionMasks();

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