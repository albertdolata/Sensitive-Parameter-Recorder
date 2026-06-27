#pragma once

#include <Adafruit_LIS3DH.h>

#include "Sensor.h"

class AccelSensor : public Sensor {
   private:
    Adafruit_LIS3DH lis3dh;
    uint8_t i2c_address;
    float axisX;
    float axisY;
    float axisZ;

   public:
    AccelSensor(uint8_t addr);

    bool initialize() override;

    void readData() override;

    float getAxisX() const;

    float getAxisY() const;

    float getAxisZ() const;
};