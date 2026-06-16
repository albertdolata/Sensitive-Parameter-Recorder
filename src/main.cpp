#include <Arduino.h>

#include "GPSManager.h"
#include "data_send_service.h"
#include "sys/time.h"
#include "SPIFFS.h"
#include "MainCentralSensorManager.h"

#define SIM_RX_PIN GPIO_NUM_18
#define SIM_TX_PIN GPIO_NUM_17
#define SIM_PWR_PIN GPIO_NUM_6

#define LED_PWR 10
#define LED_STATUS 11
#define LED_USER 12

#define PRESENCE_SENSOR_PIN 14
#define ACCEL_SENSOR_I2C_ADDR 0x18
#define PALLET_NODE_ID_1 0x01
#define PALLET_NODE_ID_2 0x02

MainCentralSensorManager centralSensorManager(PRESENCE_SENSOR_PIN, ACCEL_SENSOR_I2C_ADDR, PALLET_NODE_ID_1, PALLET_NODE_ID_2);

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

void assignSimComDataToStruct(sensor_data_t* data, GPSManager* gps) {
    data->cell_info = sim7000_get_network_params();
    data->latitude = gps->getLatitude();
    data->longitude = gps->getLongitude();
    // narazie nie ma tych czujników, ale zostawiam miejsce w strukturze i
    // kodzie, żeby łatwo było dodać w przyszłości
    // data->shock_level_palette2 = sensors[2]->value1;
    // data->shock_level_palette3 = sensors[3]->value1;
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

    pBLEScan->setAdvertisedDeviceCallbacks(centralSensorManager.getBLESensorManager());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void sendDataToServer(sensor_data_t* current_data) {
    if(!data_service_push(current_data)) {
        saveDataOffline(current_data);
    } else {
        sendBackupData();
    }
}

void checkAndUpdateTime(GPSManager* gps, uint32_t* last_gps_time) {
    if (gps->hasFix() && gps->getTimestamp() != *last_gps_time) {
        setESP32Time(gps->getTimestamp());
        *last_gps_time = gps->getTimestamp();
        Serial.println("Czas ESP32 zaktualizowany na podstawie GPS.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PWR, OUTPUT);
    pinMode(LED_STATUS, OUTPUT);
    pinMode(LED_USER, OUTPUT);

    digitalWrite(LED_PWR, HIGH);
    digitalWrite(LED_STATUS, LOW);
    digitalWrite(LED_USER, LOW);

    Serial.println("\n ----------- Rejestrator uruchomiony -----------");
    centralSensorManager.initializeAllSensors();

    SPIFFSinit();

    SIMComInit();

    GPS.begin();

    data_service_init();



    BLEInit();

}

void loop() {
    sensor_data_t current_data = {0};
    static uint32_t last_gps_time = 0;

    GPS.update();
    
    checkAndUpdateTime(&GPS, &last_gps_time);

    Serial.println("\n ------- Nasłuch BLE -------");
    digitalWrite(LED_USER, HIGH);
    BLEScanResults foundDevices = pBLEScan->start(6, false);
    pBLEScan->clearResults();
    digitalWrite(LED_USER, LOW);

    Serial.println("\n ------- Odczyt z czujników -------");
    centralSensorManager.readAllSensorsData();
    centralSensorManager.fillSensorData(&current_data);
    
    assignSimComDataToStruct(&current_data, &GPS);

    Serial.println("\n ------- Wysyłka danych -------");
    digitalWrite(LED_STATUS, HIGH);
    sendDataToServer(&current_data);
    digitalWrite(LED_STATUS, LOW);

    vTaskDelay(pdMS_TO_TICKS(5000));
}
