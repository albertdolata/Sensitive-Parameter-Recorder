#pragma once

#include "Sensor.h"
#include <Arduino.h>

class BLEPalletSensor : public Sensor {
   private:
    float axisX;
    float axisY;
    float axisZ;
    bool motionDetected;

   public:
    BLEPalletSensor() : axisX(0.0), axisY(0.0), axisZ(0.0), motionDetected(false) {}

    bool initialize() override {
        return true;
    }

    void readData() override {}

    float getAxisX() const {
        return axisX;
    }

    float getAxisY() const {
        return axisY;
    }

    float getAxisZ() const {
        return axisZ;
    }

    bool isMotionDetected() const {
        return motionDetected;
    }

    void updateDataFromBLE(int16_t x, int16_t y, int16_t z, uint8_t motion) {
        axisX = x / 16000.0f;
        axisY = y / 16000.0f;
        axisZ = z / 16000.0f;
        motionDetected = (motion == 1);
    }

};