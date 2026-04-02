#include <Arduino.h>

#include "AccelSensor.h"
#include "TempHumSensor.h"

#define NUM_SENSORS 4

Sensor* sensors[NUM_SENSORS];

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n ----------- Rejestrator uruchomiony -----------");

    sensors[0] = new TempHumSensor("bme280 Centrala Główna", 0x76);
    sensors[1] = new AccelSensor("Paleta szkło", 1);
    sensors[2] = new AccelSensor("Paleta telewizory", 2);
    sensors[3] = new AccelSensor("Paleta piwo", 3);

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->initialize();
    }
}

void loop() {
    Serial.println("\n ------- Odczyt z czujników -------");
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->readData();
    }
    delay(3000);
}
