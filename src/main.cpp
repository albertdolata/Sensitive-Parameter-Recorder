/**
 * @file main.cpp
 * @brief Główny plik wejściowy oprogramowania dla urządzenia monitorującego naczepę.
 * @details Zawiera logikę inicjalizacyjną (setup) konfigurującą magistrale (I2C, UART), 
 * podział zadań na rdzenie (Dual-Core FreeRTOS) oraz główną pętlę (loop) cyklicznie 
 * zbierającą odczyty ze wszystkich modułów i przesyłającą je na serwer MQTT.
 */

#include <Arduino.h>

#include "../include/core/system_init.h"
#include "../include/managers/GPSManager.h"
#include "../include/managers/MainCentralSensorManager.h"
#include "../include/services/ble_service.h"
#include "../include/simcom/data_send_service.h"
#include "../include/utils/time_manager.h"
#include "SPIFFS.h"
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


volatile sensor_data_t centralData = {0};   /**< Główny kontener na dane pomiarowe */
volatile uint8_t centralState = 0;          /**< Aktualny stan maszyny stanów w pętli głównej */
volatile uint32_t sleepTimeStart = 0;       /**< Znacznik czasu dla stanu uśpienia */
volatile uint32_t btsWaitStart = 0;         /**< Znacznik czasu dla negocjacji z siecią GSM */
volatile uint32_t lastCpsiCheck = 0;        /**< Zabezpieczenie przed spamowaniem komendą AT+CPSI */
volatile uint32_t gpsWaitStart = 0;         /**< Timeout dla oczekiwania na pozycję GNSS */
volatile uint32_t bleLedTimer = 0;          /**< Timer dla asynchronicznego mrugania diodą statusu BLE */
volatile uint32_t sendAttemptStart = 0;     /**< Watchdog programowy dla procesu wysyłki MQTT */
volatile uint32_t lastTimeSync = 0;         /**< Znacznik czasu ostatniej synchronizacji czasu systemowego z GNSS */

MainCentralSensorManager centralSensorManager(PRESENCE_SENSOR_PIN,
                                              ACCEL_SENSOR_I2C_ADDR,
                                              MAC_PALLETE_1,
                                              MAC_SECONDARY_CENTRAL);
GPSManager GPS;

/**
 * @brief Funkcja pomocnicza przepisująca koordynaty z obiektu GPS do struktury wysyłkowej.
 */
void assignSimComDataToStruct(sensor_data_t* data, GPSManager* gps) {
    data->latitude = gps->getLatitude();
    data->longitude = gps->getLongitude();
    data->GPS_valid = gps->isGPSValid();
}

/**
 * @brief Inicjalizuje sprzęt, usługi pokładowe oraz przypisuje zadania FreeRTOS do rdzeni.
 */
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
    SIMComInit(GPIO_NUM_18, GPIO_NUM_17, GPIO_NUM_6);
    GPS.begin();
    data_service_init();
    BLEInit(centralSensorManager.getBLESensorManager());

    xTaskCreatePinnedToCore(bleScanTask, "BLE Scan Task", 4096, NULL, 1, NULL,
                            0);
}

/**
 * @brief Główna pętla aplikacyjna (Core 1) oparta na architekturze Maszyny Stanów (Non-blocking State Machine).
 */
void loop() {
    if (centralSensorManager.BleGotPackage()) {
        digitalWrite(LED_STATUS, HIGH);
        bleLedTimer = millis();
    }

    if (digitalRead(LED_STATUS) == HIGH && (millis() - bleLedTimer >= 50)) {
        digitalWrite(LED_STATUS, LOW);
    }

    switch (centralState) {
        case 0:
            if (!GPS.isPowered()) {
                GPS.resume();
                gpsWaitStart = millis();
            }
            if (GPS.hasFix()) {
                if (lastTimeSync == 0 || (time(NULL) - lastTimeSync > 3600)) {
                    setESP32Time(GPS.getTimestamp());
                    lastTimeSync = time(NULL);
                }
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
                } else if (millis() - btsWaitStart >= 15000) {
                    centralState = 2;
                }
            }
            break;
        case 2:
            centralSensorManager.readAllSensorsData();
            centralSensorManager.fillSensorData((sensor_data_t*)&centralData);
            assignSimComDataToStruct((sensor_data_t*)&centralData, &GPS);
            centralData.timestamp = time(NULL);
            centralState = 3;
            break;
        case 3:
            digitalWrite(LED_USER, HIGH);
            data_service_push((sensor_data_t*)&centralData);
            sendAttemptStart = millis();
            centralState = 4;
            break;
        case 4:
            if (!data_service_is_active()) {
                digitalWrite(LED_USER, LOW);
                sleepTimeStart = millis();
                centralState = 5;
            } else if (millis() - sendAttemptStart >= 180000) {
                ESP.restart();
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
