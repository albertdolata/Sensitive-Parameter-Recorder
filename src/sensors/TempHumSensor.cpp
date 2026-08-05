/**
 * @file TempHumSensor.cpp
 * @brief Plik źródłowy zawierający implementację metod klasy TempHumSensor.
 */

#include "../include/sensors/TempHumSensor.h"

TempHumSensor::TempHumSensor() : temperature(0.0), humidity(0.0) {}

bool TempHumSensor::initialize() {
    return adafruit_si7021.begin();
}

void TempHumSensor::readData() {
    temperature = adafruit_si7021.readTemperature();
    humidity = adafruit_si7021.readHumidity();
}

float TempHumSensor::getTemperature() const {
    return temperature;
}

float TempHumSensor::getHumidity() const {
    return humidity;
}
