/**
 * @file AccelSensor.cpp
 * @brief Plik źródłowy zawierający definicje metod klasy AccelSensor
 */

#include "../include/sensors/AccelSensor.h"

AccelSensor::AccelSensor(uint8_t addr)
    : i2c_address(addr), axisX(0.0), axisY(0.0), axisZ(0.0) {}

bool AccelSensor::initialize() {
    return lis3dh.begin(i2c_address);
}

void AccelSensor::readData() {
    if (lis3dh.haveNewData()) {
        lis3dh.read();
        axisX = lis3dh.x_g;
        axisY = lis3dh.y_g;
        axisZ = lis3dh.z_g;
    }
}

float AccelSensor::getAxisX() const {
    return axisX;
}

float AccelSensor::getAxisY() const {
    return axisY;
}

float AccelSensor::getAxisZ() const {
    return axisZ;
}