#include "tusb_config.h"
#include "hal/System/MutexInterface.hpp"
#include "hal/System/LockGuard.hpp"
#include <vector>

// CDC is used to create a debug serial interface

void cdc_init(MutexInterface* cdcStdioMutex);
void cdc_task();
