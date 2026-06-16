#pragma once

#include "Sensor.h"

class PresenceSensor : public Sensor {
   private:
    bool isSomeOneHere;
    uint8_t GPIO_pin;


   public:
    PresenceSensor(uint8_t pin) : GPIO_pin(pin), isSomeOneHere(false) {}

    bool initialize() override {
        pinMode(GPIO_pin, INPUT);
        return true;
    }

    void readData() override {
        isSomeOneHere = (digitalRead(GPIO_pin) == HIGH);
    }
    bool getPresence() const {
        return isSomeOneHere;
    }
};
