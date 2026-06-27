#pragma once

#include <Arduino.h>

#include "Sensor.h"

class BLESecondaryCentral : public Sensor {
   private:
    float temp;
    float hum;
    bool isClosed;

   public:
    BLESecondaryCentral();

    bool initialize() override;

    void readData() override;

    float getTemp() const;

    float getHumidity() const;

    bool getIsClosed() const;

    void updateDataFromBLE(int16_t temp, uint16_t hum, uint8_t isClosed);
};