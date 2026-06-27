#pragma once

#include <Adafruit_Si7021.h>

#include "Sensor.h"

class TempHumSensor : public Sensor {
   private:
    Adafruit_Si7021 adafruit_si7021;
    float temperature;
    float humidity;

   public:
    TempHumSensor();

    bool initialize() override;

    void readData() override;

    float getTemperature() const;

    float getHumidity() const;
};