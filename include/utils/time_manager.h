#pragma once

#include <Arduino.h>
#include "../managers/GPSManager.h"

void setESP32Time(uint32_t timestamp);

void checkAndUpdateTime(GPSManager* gps, uint32_t* last_gps_time);