#pragma once

#include "Sensor.h"
#include <Adafruit_Si7021.h>


class TempHumSensor : public Sensor {
   private:
    Adafruit_Si7021 adafruit_si7021;
    float temperature;
    float humidity;

   public:
    TempHumSensor() : temperature(0.0), humidity(0.0) {}

    bool initialize() override {
        return adafruit_si7021.begin();
    }

    void readData() override {
        temperature = adafruit_si7021.readTemperature();
        humidity = adafruit_si7021.readHumidity();
    }

    float getTemperature() const {
        return temperature;
    }

    float getHumidity() const {
        return humidity;
    }
};