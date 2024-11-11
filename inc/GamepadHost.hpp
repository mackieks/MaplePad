#pragma once

#include <stdint.h>

class GamepadHost
{
public:
    inline GamepadHost() {}
    inline virtual ~GamepadHost() {}

    //! Enumerates hat positions
    enum class Hat : uint8_t
    {
        NEUTRAL = 0,
        UP,
        UP_RIGHT,
        RIGHT,
        DOWN_RIGHT,
        DOWN,
        DOWN_LEFT,
        LEFT,
        UP_LEFT
    };

    //! Standard set of gamepad controls
    struct Controls
    {
        Hat hat;
        bool west;
        bool south;
        bool east;
        bool north;

        bool l1;
        bool r1;
        uint8_t l2;
        uint8_t r2;
        bool l3;
        bool r3;

        bool start;
        bool menu;

        uint8_t lx;
        uint8_t ly;
        uint8_t rx;
        uint8_t ry;
    };

    //! Set updated controls
    //! @param[in] controls  Updated controls
    virtual void setControls(const Controls& controls) = 0;
};

void set_gamepad_host(GamepadHost* ctrlr);
