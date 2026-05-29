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
#include "sys/time.h"
#include "SPIFFS.h"

#define NUM_SENSORS 6
#define SIM_RX_PIN GPIO_NUM_16
#define SIM_TX_PIN GPIO_NUM_17
#define SIM_PWR_PIN GPIO_NUM_27

Sensor* sensors[NUM_SENSORS];
BLESensorManager* radarBLE;
BLEScan* pBLEScan;
GPSManager GPS;


void setESP32Time(uint32_t timestamp) {
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

void SPIFFSinit(){
    if(!SPIFFS.begin(true)) {
        ESP.restart();
    }
}

void saveDataOffline(sensor_data_t* data){
    File dataFile = SPIFFS.open("/data_backup.dat", FILE_APPEND);
    if (!dataFile) {
        Serial.println("Błąd otwierania pliku do zapisu danych offline!");
        return;
    } else {
        dataFile.write((uint8_t*)data, sizeof(sensor_data_t));
        dataFile.close();
        Serial.println("Dane zapisane offline pomyślnie.");
    }
}

void sendBackupData() {
    if(!SPIFFS.exists("/data_backup.dat")) {
        return;
    }
    File dataFile = SPIFFS.open("/data_backup.dat", FILE_READ);
    if(!dataFile) {
        Serial.println("Błąd otwierania pliku z danymi offline!");
        return;
    }

    if(dataFile.size() == 0) {
        dataFile.close();
        SPIFFS.remove("/data_backup.dat");
        return;
    }

    File tempFile = SPIFFS.open("/temp_backup.dat", FILE_WRITE);
    if(!tempFile) {
        Serial.println("Błąd otwierania tymczasowego pliku do zapisu danych offline!");
        dataFile.close();
        return;
    }

    sensor_data_t offlineData = {0};
    bool QueueFull = false;
    int dataNotSend = 0;


    while(dataFile.read((uint8_t*)&offlineData, sizeof(sensor_data_t)) == sizeof(sensor_data_t)) {
        if(!QueueFull) {
            if(!data_service_push(&offlineData)) {
                QueueFull = true;
                tempFile.write((uint8_t*)&offlineData, sizeof(sensor_data_t));
                dataNotSend++;
            }
        } else {
            tempFile.write((uint8_t*)&offlineData, sizeof(sensor_data_t));
            dataNotSend++;
        }
    }
    
    dataFile.close();
    tempFile.close();
    SPIFFS.remove("/data_backup.dat");

    if(dataNotSend > 0) {
        Serial.print("Liczba nie wysłanych danych offline: ");
        Serial.println(dataNotSend);
        SPIFFS.rename("/temp_backup.dat", "/data_backup.dat");
    } else {
        SPIFFS.remove("/temp_backup.dat");
        Serial.println("Wszystkie dane offline zostały wysłane pomyślnie.");
    }
}

void assignDataToStruct(sensor_data_t* data, GPSManager* gps, Sensor* sensors[]) {
    data->cell_info = sim7000_get_network_params();
    data->latitude = gps->getLatitude();
    data->longitude = gps->getLongitude();
    data->temperature_main_central = sensors[0]->value1;
    data->humidity_main_central = sensors[0]->value2;
    data->shock_level_main_central = sensors[1]->value1;
    data->shock_level_palette1 = sensors[1]->value1;
    // narazie nie ma tych czujników, ale zostawiam miejsce w strukturze i
    // kodzie, żeby łatwo było dodać w przyszłości
    // data->shock_level_palette2 = sensors[2]->value1;
    // data->shock_level_palette3 = sensors[3]->value1;
    data->temperature_secondary_central = 0;
    data->humidity_secondary_central = 0;
    data->shock_level_secondary_central = 0;
    data->timestamp = time(NULL);
}

void SIMComInit() {
    if (!sim7000_init(SIM_RX_PIN, SIM_TX_PIN, SIM_PWR_PIN)) {
        Serial.println("Błąd inicjalizacji modemu SIM7000. Restartowanie...");
        ESP.restart();
    }

    if (!sim7000_wait_for_network()) {
        Serial.println("Nie można połączyć z siecią. Restartowanie...");
        ESP.restart();
    }
}

void BLEInit() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    radarBLE = new BLESensorManager(sensors[1], sensors[2], sensors[3]);

    pBLEScan->setAdvertisedDeviceCallbacks(radarBLE);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void SensorTableInit(Sensor* sensor[]) {
    sensor[0] = new TempHumSensor("bme280 Centrala Główna", 0x76);
    sensor[1] = new BLESensor("Paleta szkło");
    sensor[2] = new BLESensor("Paleta telewizory");
    sensor[3] = new BLESensor("Paleta piwo");
    sensor[4] = new ContactSensor("Kontaktron");
    sensor[5] = new PresenceSensor("Czujnik obecności człowieka");

    for (int i = 0; i < NUM_SENSORS; i++) {
        sensor[i]->initialize();
    }
}

void readDataFromSensor(Sensor* sensor[]) {
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensor[i]->readData();
    }
}

void sendDataToServer(sensor_data_t* current_data) {
    if(!data_service_push(current_data)) {
        saveDataOffline(current_data);
    } else {
        sendBackupData();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n ----------- Rejestrator uruchomiony -----------");

    SPIFFSinit();

    SIMComInit();

    GPS.begin();

    data_service_init();

    SensorTableInit(sensors);

    BLEInit();

}

void loop() {
    sensor_data_t current_data = {0};
    static uint32_t last_gps_time = 0;

    GPS.update();
    if(GPS.hasFix()) {
        uint32_t current_gps_time = GPS.getTimestamp();
        if(current_gps_time != last_gps_time) {
            setESP32Time(current_gps_time);
            last_gps_time = current_gps_time;
        }
    }

    Serial.println("\n ------- Nasłuch BLE -------");
    BLEScanResults foundDevices = pBLEScan->start(6, false);
    pBLEScan->clearResults();

    Serial.println("\n ------- Odczyt z czujników -------");
    readDataFromSensor(sensors);

    assignDataToStruct(&current_data, &GPS, sensors);

    Serial.println("\n ------- Wysyłka danych -------");
    sendDataToServer(&current_data);

    vTaskDelay(pdMS_TO_TICKS(5000));
}
