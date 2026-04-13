#pragma once
#include <Sensor.h>

class BLESensor : public Sensor {
   public:
    BLESensor(String name) : Sensor(name) {}

    bool initialize() override {
        return true;
    }

    void readData() override {}
};