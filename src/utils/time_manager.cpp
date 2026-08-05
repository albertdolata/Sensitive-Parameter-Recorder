#include "../include/utils/time_manager.h"

void setESP32Time(uint32_t timestamp) {
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

void checkAndUpdateTime(GPSManager* gps, uint32_t* last_gps_time) {
    if (gps->hasFix() && gps->getTimestamp() != *last_gps_time) {
        setESP32Time(gps->getTimestamp());
        *last_gps_time = gps->getTimestamp();
    }
}
