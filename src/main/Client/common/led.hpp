#pragma once

#include <stdint.h>

void led_init();
void led_task(uint64_t lastActivityTimeUs);
