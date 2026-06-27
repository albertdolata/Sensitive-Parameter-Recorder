#pragma once

#include <Arduino.h>

#include "Sensor.h"

class PresenceSensor : public Sensor {
   private:
    bool isSomeOneHere;
    uint8_t GPIO_pin;

   public:
    PresenceSensor(uint8_t pin);

    bool initialize() override;

    void readData() override;

    bool getPresence() const;
};
