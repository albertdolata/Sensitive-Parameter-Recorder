#pragma once

#include "Sensor.h"

class BLEPalletSensor : public Sensor {
   private:
    float axisX;
    float axisY;
    float axisZ;

   public:
    BLEPalletSensor() : axisX(0.0), axisY(0.0), axisZ(0.0) {}

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

    void updateDataFromBLE(float x, float y, float z) {
        axisX = x;
        axisY = y;
        axisZ = z;
    }

};