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
    AccelSensor(uint8_t addr)
        : i2c_address(addr), axisX(0.0), axisY(0.0), axisZ(0.0) {}

    bool initialize() override {
        return lis3dh.begin(i2c_address);
        //lis3dh.setRange(LIS3DH_RANGE_2_G); //do ustalenia zakres pomiarowy
    }

    void readData() override {
        if (lis3dh.haveNewData()) {
            lis3dh.read();
            axisX = lis3dh.x_g;
            axisY = lis3dh.y_g;
            axisZ = lis3dh.z_g;
        }
    }

    float getAxisX() const {
        return axisX;
    }
    float getAxisY() const {
        return axisY;
    }
    float getAxisZ() const {
        return axisZ;
    }
};