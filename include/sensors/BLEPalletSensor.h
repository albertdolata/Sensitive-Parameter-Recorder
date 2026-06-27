#pragma once

#include <Arduino.h>

#include "Sensor.h"

class BLEPalletSensor : public Sensor {
   private:
    float axisX;
    float axisY;
    float axisZ;
    bool motionDetected;

   public:
    BLEPalletSensor();

    bool initialize() override;

    void readData() override;

    float getAxisX() const;

    float getAxisY() const;

    float getAxisZ() const;

    bool isMotionDetected() const;

    void updateDataFromBLE(int16_t x, int16_t y, int16_t z, uint8_t motion);
};