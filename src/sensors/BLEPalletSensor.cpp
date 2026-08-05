/**
 * @file BLEPalletSensor.cpp
 * @brief Plik źródłowy zawierający definicje metod klasy BLEPalletSensor.
 */

#include "../include/sensors/BLEPalletSensor.h"

BLEPalletSensor::BLEPalletSensor()
    : axisX(0.0), axisY(0.0), axisZ(0.0), motionDetected(false) {}

bool BLEPalletSensor::initialize() {
    return true;
}

void BLEPalletSensor::readData() {}

float BLEPalletSensor::getAxisX() const {
    return axisX;
}

float BLEPalletSensor::getAxisY() const {
    return axisY;
}

float BLEPalletSensor::getAxisZ() const {
    return axisZ;
}

bool BLEPalletSensor::isMotionDetected() const {
    return motionDetected;
}

void BLEPalletSensor::updateDataFromBLE(int16_t x, int16_t y, int16_t z,
                                        uint8_t motion) {
    axisX = x / 16000.0f;
    axisY = y / 16000.0f;
    axisZ = z / 16000.0f;
    motionDetected = (motion == 1);
}