#pragma once
#include "Sensor.h"

class PresenceSensor : public Sensor {
   private:
    bool isSomeOneHere;

   public:
    PresenceSensor(String name) : Sensor(name) {}

    bool initialize() override {
        return true;
    }

    void readData() override {
        isSomeOneHere = random(0, 2);
        if (isSomeOneHere)
            Serial.printf("UWAGA! %s wykrył, że ktoś jest w naczepie! \n",
                          sensorName.c_str());
    }
};
