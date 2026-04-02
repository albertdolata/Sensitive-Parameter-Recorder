#include <Arduino.h>

#include "AccelSensor.h"
#include "TempHumSensor.h"

Sensor* sensors[3];

void setup() {
    Serial.begin(115200);

    sensors[0] = new TempHumSensor("bme280 Centrala Główna", 0x76);
    sensors[1] = new AccelSensor("Paleta szkło", 1);
    sensors[2] = new AccelSensor("Paleta telewizory", 2);
    sensors[3] = new AccelSensor("Paleta piwo", 3);

    for (int i = 0; i < 3; i++) {
        sensors[i]->initialize();
    }
}

void loop() {
    for (int i = 0; i < 3; i++) {
        sensors[i]->readData();
        delay(3000);
    }
}
