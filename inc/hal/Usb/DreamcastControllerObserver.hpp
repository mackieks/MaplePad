#ifndef __DREAMCAST_CONTROLLER_OBSERVER_H__
#define __DREAMCAST_CONTROLLER_OBSERVER_H__

#include <stdint.h>
#include "dreamcast_structures.h"

//! This interface is used to decouple the USB functionality in HAL from the Dreamcast functionality
class DreamcastControllerObserver
{
    public:
        typedef controller_condition_t ControllerCondition;
        typedef vmu_timer_condition_t SecondaryControllerCondition;

        //! Sets the current Dreamcast controller condition
        //! @param[in] controllerCondition  The current condition of the Dreamcast controller
        virtual void setControllerCondition(const ControllerCondition& controllerCondition) = 0;

        //! Sets the current Dreamcast secondary controller condition
        //! @param[in] secondaryControllerCondition  The current secondary condition of the
        //!                                          Dreamcast controller
        virtual void setSecondaryControllerCondition(
            const SecondaryControllerCondition& secondaryControllerCondition) = 0;

        //! Called when controller connected
        virtual void controllerConnected() = 0;

        //! Called when controller disconnected
        virtual void controllerDisconnected() = 0;
};

#endif // __DREAMCAST_CONTROLLER_OBSERVER_H__
