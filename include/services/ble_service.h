#pragma once

#include <Arduino.h>
#include "../managers/BLESensorManager.h"

void BLEInit(BLESensorManager* bleManager);

void bleScanTask(void* parameter);