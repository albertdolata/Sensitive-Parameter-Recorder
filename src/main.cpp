#include <Arduino.h>

#include "AccelSensor.h"
#include "TempHumSensor.h"
#include "ContactSensor.h"
#include "PresenceSensor.h"
#include "CloudManager.h"
#include "secrets.h"
#include "BLESensor.h"
#include "BLESensorManager.h"

#define NUM_SENSORS 7

Sensor* sensors[NUM_SENSORS];
CloudManager cloud;
BLESensorManager* radarBLE;
BLEScan* pBLEScan;

volatile int serverRespone;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n ----------- Rejestrator uruchomiony -----------");

    cloud.connectWiFi(ssid, password);
    cloud.initCloud();

    sensors[0] = new TempHumSensor("bme280 Centrala Główna", 0x76);
    sensors[1] = new AccelSensor("Paleta szkło", 1);
    sensors[2] = new AccelSensor("Paleta telewizory", 2);
    sensors[3] = new AccelSensor("Paleta piwo", 3);
    sensors[4] = new ContactSensor("Kontaktron");
    sensors[5] = new PresenceSensor("Czujnik obecności człowieka");
    sensors[6] = new BLESensor("NRF1");

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->initialize();
    }

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    radarBLE = new BLESensorManager(sensors[6]);
}

void loop() {
    Serial.println("\n ------- Odczyt z czujników -------");
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->readData();
    }

    ThingSpeak.setField(1, sensors[0]->value1); //temperatura z BME
    ThingSpeak.setField(2, sensors[0]->value3); //wilgotność z BME
    ThingSpeak.setField(3, sensors[0]->value2); //ciśnienie z BME
    ThingSpeak.setField(4, sensors[1]->value1); //wstrzas z paleta szkło
    ThingSpeak.setField(5, sensors[2]->value1); //wstrzas z paleta telewizroy
    ThingSpeak.setField(6, sensors[3]->value1); //wstrzas z paleta piwo
    ThingSpeak.setField(7, sensors[4]->value1); //kontaktron
    ThingSpeak.setField(8, sensors[5]->value1); //obecność człowieka

    Serial.println("Wysyłanie danych do ThingSpeak");
    serverRespone = ThingSpeak.writeFields(channelNumber, writeAPIKey);
    if( serverRespone == 200)
    Serial.println("Dane zostały wysłane");
    else
    Serial.printf("Coś poszło nie tak, kod błędu: %d", serverRespone);

    delay(20000); // tak dużo bo ThingSpeak na darmowym konicie wysyła co 15 sek dane
}
