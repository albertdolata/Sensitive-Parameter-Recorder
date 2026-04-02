#pragma once
#include "Sensor.h"

class ContactSensor : public Sensor {
   private:
    bool isDoorOpen;

   public:
    ContactSensor(String name) : Sensor(name) {}

    bool initialize() override {
        return true;
    }

    void readData() override {
        isDoorOpen = random(0, 2);
        value1 = isDoorOpen;
        if (isDoorOpen)
            Serial.printf("UWAGA! %s wykrył, że drzwi są otwarte \n",
                          sensorName.c_str());
    }
};
