#pragma once
#include <Arduino.h>

#include "../simcom/sim7070_core.h"

class GPSManager {
   private:
    double latitude;
    double longitude;
    uint32_t timestamp;
    bool isON;
    bool fixStatus;

    void parseTime(String rawTime);

    void parseResponse(String response);

   public:
    GPSManager();

    void begin();

    void update();

    void pause();

    void resume();

    double getLatitude();

    double getLongitude();

    uint32_t getTimestamp();

    bool hasFix();

    bool isPowered();
};