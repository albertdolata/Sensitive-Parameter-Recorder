#include <Arduino.h>

#include "GPSManager.h"
#include "MainCentralSensorManager.h"
#include "SPIFFS.h"
#include "data_send_service.h"
#include "esp_log.h"
#include "sys/time.h"

#define SIM_RX_PIN GPIO_NUM_18
#define SIM_TX_PIN GPIO_NUM_17
#define SIM_PWR_PIN GPIO_NUM_6

#define LED_PWR 10
#define LED_STATUS 11
#define LED_USER 12

#define PRESENCE_SENSOR_PIN 14
#define ACCEL_SENSOR_I2C_ADDR 0x18
#define MAC_PALLETE_1 "d1:d2:e4:92:0e:c4"
#define MAC_SECONDARY_CENTRAL "ca:37:e3:06:9b:84"

volatile sensor_data_t centralData = {0};
volatile uint8_t centralState = 0;
volatile uint32_t sleepTimeStart = 0;
volatile uint32_t btsWaitStart = 0;
volatile uint32_t lastCpsiCheck = 0;
volatile uint32_t gpsWaitStart = 0;

MainCentralSensorManager centralSensorManager(PRESENCE_SENSOR_PIN,
                                              ACCEL_SENSOR_I2C_ADDR,
                                              MAC_PALLETE_1,
                                              MAC_SECONDARY_CENTRAL);

BLEScan* pBLEScan;
GPSManager GPS;

bool initial_fix_acquired = false;

void bleScanTask(void* parameter) {
    while (true) {
        BLEScanResults foundDevices = pBLEScan->start(6, false);
        pBLEScan->clearResults();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void setESP32Time(uint32_t timestamp) {
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

void SPIFFSinit() {
    if (!SPIFFS.begin(true)) {
        ESP.restart();
    }
}

void assignSimComDataToStruct(sensor_data_t* data, GPSManager* gps) {
    data->latitude = gps->getLatitude();
    data->longitude = gps->getLongitude();
}

void SIMComInit() {
    if (!sim7070_init(SIM_RX_PIN, SIM_TX_PIN, SIM_PWR_PIN)) {
        ESP.restart();
    }

    if (!sim7070_wait_for_network()) {
        ESP.restart();
    }
}

void BLEInit() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();

    pBLEScan->setAdvertisedDeviceCallbacks(
        centralSensorManager.getBLESensorManager());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void checkAndUpdateTime(GPSManager* gps, uint32_t* last_gps_time) {
    if (gps->hasFix() && gps->getTimestamp() != *last_gps_time) {
        setESP32Time(gps->getTimestamp());
        *last_gps_time = gps->getTimestamp();
    }
}

void setup() {
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("SIM7070_GPRS", ESP_LOG_ERROR);
    delay(1000);

    pinMode(LED_PWR, OUTPUT);
    pinMode(LED_STATUS, OUTPUT);
    pinMode(LED_USER, OUTPUT);

    digitalWrite(LED_PWR, HIGH);
    digitalWrite(LED_STATUS, LOW);
    digitalWrite(LED_USER, LOW);

    centralSensorManager.initializeAllSensors();

    SPIFFSinit();

    SIMComInit();

    GPS.begin();

    data_service_init();

    BLEInit();

    xTaskCreatePinnedToCore(bleScanTask, "BLE Scan Task", 4096, NULL, 1, NULL,
                            0);
}

void loop() {
    switch (centralState) {
        case 0:
            if (!GPS.isPowered()) {
                GPS.resume();
                gpsWaitStart = millis();
            }
            if (GPS.hasFix()) {
                setESP32Time(GPS.getTimestamp());
                GPS.pause();
                btsWaitStart = millis();
                centralState = 1;
            } else if (millis() - gpsWaitStart >= 120000) { 
                GPS.pause();
                btsWaitStart = millis();
                centralState = 1;
            } else {
                GPS.update();
            }
            break;
        case 1:
            if (millis() - lastCpsiCheck >= 1000) {
                    lastCpsiCheck = millis();
                    cell_info_t cell = sim7070_get_network_params();
                    if (cell.is_valid) {
                        centralData.cell_info.mcc = cell.mcc;
                        centralData.cell_info.mnc = cell.mnc;
                        centralData.cell_info.tac = cell.tac;
                        centralData.cell_info.cid = cell.cid;
                        centralData.cell_info.is_valid = cell.is_valid;
                        centralState = 2;
                    } 
                    else if (millis() - btsWaitStart >= 15000) { 
                        centralState = 2;
                    }
                }
            break;
        case 2:
            centralSensorManager.readAllSensorsData();
            centralSensorManager.fillSensorData((sensor_data_t*)&centralData);
            centralData.timestamp = time(NULL);
            centralState = 3;
            break;
        case 3:
            assignSimComDataToStruct((sensor_data_t*)&centralData, &GPS);
            data_service_push((sensor_data_t*)&centralData);
            centralState = 4;
            break;
        case 4:
            if (!data_service_is_busy()) {
                sleepTimeStart = millis();
                centralState = 5;
            }
            break;
        case 5:
            if (millis() - sleepTimeStart >= 10000) {
                gpsWaitStart = millis();
                centralState = 0;
            }
            break;
        default:
            break;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
