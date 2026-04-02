#pragma once

#include "Sensor.h"

class AccelSensor : public Sensor {
   private:
    int idPalete;

   public:
    AccelSensor(String name, int id) : Sensor(name) {
        idPalete = id;
    }

    bool initialize() override {
        return true;
    }

    void readData() override {
        int shocklvl = random(0, 100);
        if (shocklvl > 50) {
            Serial.printf("Paleta %s -> Zarejestrowano wstrząs o sile: %.2d \n",
                          sensorName.c_str(), shocklvl);
        } else {
            Serial.println("nie zarejestrowano gwałtownego wstrząsu");
        }
    }
};