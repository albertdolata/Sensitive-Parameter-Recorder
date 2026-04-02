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
        value1 = shocklvl;
        if (shocklvl > 50) {
            Serial.printf("Paleta nr %d %s -> Zarejestrowano wstrząs o sile: %.2d \n",
                          idPalete, sensorName.c_str(), shocklvl);
        } else {
            Serial.println("nie zarejestrowano gwałtownego wstrząsu");
        }
    }
};