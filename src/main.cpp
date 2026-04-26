#include <Arduino.h>

#include "AccelSensor.h"
#include "BLESensor.h"
#include "BLESensorManager.h"
#include "CloudManager.h"
#include "ContactSensor.h"
#include "GPSManager.h"
#include "PresenceSensor.h"
#include "TempHumSensor.h"
#include "secrets.h"

#define NUM_SENSORS 6

Sensor* sensors[NUM_SENSORS];
CloudManager cloud;
BLESensorManager* radarBLE;
BLEScan* pBLEScan;
GPSManager GPS(Serial2, 16, 17, 19200);

unsigned long lastGpsRequest = 0;

volatile int serverRespone;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n ----------- Rejestrator uruchomiony -----------");

    cloud.connectWiFi(ssid, password);
    cloud.initCloud();
    GPS.begin();

    sensors[0] = new TempHumSensor("bme280 Centrala Główna", 0x76);
    sensors[1] = new BLESensor("Paleta szkło");
    sensors[2] = new BLESensor("Paleta telewizory");
    sensors[3] = new BLESensor("Paleta piwo");
    sensors[4] = new ContactSensor("Kontaktron");
    sensors[5] = new PresenceSensor("Czujnik obecności człowieka");

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->initialize();
    }

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    radarBLE = new BLESensorManager(sensors[1], sensors[2], sensors[3]);

    pBLEScan->setAdvertisedDeviceCallbacks(radarBLE);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastGpsRequest >= 10000) {
        lastGpsRequest = millis();
        GPS.requestPosition();
    }

    GPS.update();

    if (GPS.hasFix()) {
        Serial.print("----------------GPS ON----------------\n");
        Serial.print("Lat: ");
        Serial.print(GPS.getLatitude(), 6);
        Serial.print("Lon: ");
        Serial.print(GPS.getLongtitude(), 6);
    }
    Serial.println("\n ------- Nasłuch BLE -------");
    BLEScanResults foundDevices = pBLEScan->start(6, false);
    pBLEScan->clearResults();

    Serial.println("\n ------- Odczyt z czujników -------");
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->readData();
    }

    ThingSpeak.setField(1, sensors[0]->value1);  // temperatura z BME
    ThingSpeak.setField(2, sensors[0]->value2);  // wilgotność z BME
    ThingSpeak.setField(3, sensors[1]->value1);  // Temperatura z paleta szkło BLESensor
    ThingSpeak.setField(4, sensors[2]->value2);  // Wilgotność z paleta szkło BLESensor
    ThingSpeak.setField(5, sensors[3]->value3);  // Wychylenie z paleta telewizroy BLESensor
    ThingSpeak.setField(6, sensors[2]->value1);  // Temperatura z paleta piwo BLESensor
    ThingSpeak.setField(7, sensors[4]->value1);  // kontaktron (mocked)
    ThingSpeak.setField(8, sensors[5]->value1);  // obecność człowieka (mocked)

    if (GPS.hasFix()) {
        ThingSpeak.setLatitude(GPS.getLatitude());
        ThingSpeak.setLongitude(GPS.getLongtitude());
    }

    Serial.println("Wysyłanie danych do ThingSpeak");
    serverRespone = ThingSpeak.writeFields(channelNumber, writeAPIKey);
    if (serverRespone == 200)
        Serial.println("Dane zostały wysłane");
    else
        Serial.printf("Coś poszło nie tak, kod błędu: %d", serverRespone);

    delay(20000);  // tak dużo bo ThingSpeak na darmowym konicie wysyła co 15
                   // sek dane
}
