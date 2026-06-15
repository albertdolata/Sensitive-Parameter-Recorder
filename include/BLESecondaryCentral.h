#pragma once

#include "Sensor.h"
#include <Arduino.h>

class BLESecondaryCentral : public Sensor {
   private:
    float temp;
    float hum;
    bool isClosed;

   public:
    BLESecondaryCentral() : temp(0.0), hum(0.0), isClosed(false) {}

    bool initialize() override {
        return true;
    }

    void readData() override {}

    float getTemp() const {
        return temp;
    }

    float getHumidity() const {
        return hum;
    }

    bool getIsClosed() const {
        return isClosed;
    }

    void updateDataFromBLE(int16_t temp, uint16_t hum, uint8_t isClosed) {
        this->temp = temp / 100.0f;
        this->hum = hum / 100.0f ;
        this->isClosed = (isClosed == 1);
    }

};