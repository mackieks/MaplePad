#include "led.hpp"

#include "configuration.h"

#include "pico/stdlib.h"

// Only a single LED function for client
#define CLIENT_LED_PIN ((USB_LED_PIN >= 0) ? USB_LED_PIN : SIMPLE_USB_LED_PIN)

void led_init()
{
    if (CLIENT_LED_PIN >= 0)
    {
        gpio_init(CLIENT_LED_PIN);
        gpio_set_dir_out_masked(1<<CLIENT_LED_PIN);
    }
}

void led_task(uint64_t lastActivityTimeUs)
{
    static bool ledOn = false;
    static uint64_t startUs = 0;
    static const uint32_t BLINK_TIME_US = 250000;
    static const uint32_t ACTIVITY_DELAY_US = 500000;

    // To correct for the non-atomic read in getLastActivityTime(), only update activityStopTime
    // if a new time is greater
    uint64_t currentTime = time_us_64();
    static uint64_t activityStopTime = 0;
    if (lastActivityTimeUs != 0)
    {
        lastActivityTimeUs += ACTIVITY_DELAY_US;
        if (lastActivityTimeUs > activityStopTime)
        {
            activityStopTime = lastActivityTimeUs;
        }
    }

    if (activityStopTime > currentTime)
    {
        uint64_t t = currentTime - startUs;
        if (t >= BLINK_TIME_US)
        {
            startUs += BLINK_TIME_US;
            ledOn = !ledOn;
        }
    }
    else
    {
        startUs = currentTime;
        ledOn = true;
    }

    gpio_put(CLIENT_LED_PIN, ledOn);
}
