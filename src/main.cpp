#include <Arduino.h>

#include "AccelSensor.h"
#include "BLESensor.h"
#include "BLESensorManager.h"
#include "ContactSensor.h"
#include "GPSManager.h"
#include "PresenceSensor.h"
#include "TempHumSensor.h"
#include "data_send_service.h"
#include "secrets.h"

#define NUM_SENSORS 6
#define SIM_RX_PIN GPIO_NUM_16
#define SIM_TX_PIN GPIO_NUM_17
#define SIM_PWR_PIN GPIO_NUM_27

Sensor* sensors[NUM_SENSORS];
BLESensorManager* radarBLE;
BLEScan* pBLEScan;
GPSManager GPS;

unsigned long lastGpsRequest = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n ----------- Rejestrator uruchomiony -----------");

    if (!sim7000_init(SIM_RX_PIN, SIM_TX_PIN, SIM_PWR_PIN)) {
        Serial.println("Błąd inicjalizacji modemu SIM7000. Restartowanie...");
        ESP.restart();
    }

    if (!sim7000_wait_for_network()) {
        Serial.println("Nie można połączyć z siecią. Restartowanie...");
        ESP.restart();
    }

    GPS.begin();

    data_service_init();

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
    sensor_data_t current_data = {0};

    GPS.update();

    Serial.println("\n ------- Nasłuch BLE -------");
    BLEScanResults foundDevices = pBLEScan->start(6, false);
    pBLEScan->clearResults();

    Serial.println("\n ------- Odczyt z czujników -------");
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensors[i]->readData();
    }

    current_data.cell_info = sim7000_get_network_params();
    current_data.latitude = GPS.getLatitude();
    current_data.longitude = GPS.getLongitude();
    current_data.temperature_main_central = sensors[0]->value1;
    current_data.humidity_main_central = sensors[0]->value2;
    current_data.shock_level_main_central = sensors[0]->value3;
    current_data.shock_level_palette1 = sensors[1]->value1;
    // narazie nie ma tych czujników, ale zostawiam miejsce w strukturze i
    // kodzie, żeby łatwo było dodać w przyszłości
    // current_data.shock_level_palette2 = sensors[2]->value1;
    // current_data.shock_level_palette3 = sensors[3]->value1;
    current_data.temperature_secondary_central = 0;
    current_data.humidity_secondary_central = 0;
    current_data.shock_level_secondary_central = 0;

    if (data_service_push(&current_data)) {
        Serial.println("Dane dodane do kolejki wysyłkowej.");
    } else {
        Serial.println("Kolejka wysyłkowa pełna! Dane odrzucone.");
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
}
