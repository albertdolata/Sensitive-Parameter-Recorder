#pragma once

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "Sensor.h"

class TempHumSensor : public Sensor {
   private:
    Adafruit_BME280 bme;
    uint8_t i2cAddress;

   public:
    TempHumSensor(String name, uint8_t address) : Sensor(name) {
        i2cAddress = address;
    }

    bool initialize() override {
        if (bme.begin(i2cAddress))
            return true;
        else
            return false;
    }

    void readData() override {
        Serial.printf(
            "%s -> Temperatura: %.2f *C | Wilgotność: %.2f %% | Ciśnienie: "
            "%.2f hPa\n",
            sensorName.c_str(), bme.readTemperature(), bme.readHumidity(),
            bme.readPressure() / 100.0F);
    }
};