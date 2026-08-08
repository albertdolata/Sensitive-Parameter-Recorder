/**
 * @file data_send_service.cpp
 * @brief Plik źródłowy implementujący logikę wysyłania i archiwizacji danych (SPIFFS).
 */

#include "../include/simcom/data_send_service.h"

#include <string.h>

#include "../include/simcom/sim7070_core.h"
#include "SPIFFS.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/** 
 * @brief Maksymalna dopuszczalna liczba rekordów w pliku offline. 
 * @details Zabezpiecza pamięć Flash (SPIFFS) przed całkowitym zapełnieniem. 
 * Przy średnim rozmiarze struktury zapobiega to uszkodzeniu systemu plików.
 */
#define MAX_BACKLOG_RECORDS 45000

/** 
 * @brief Liczba rekordów pozostawiana po wykonaniu operacji przycinania (Trim).
 * @details Tworzy tzw. histerezę (margines 5000 rekordów), aby unikać 
 * obciążającego rotowania pliku z każdym kolejnym nowym pomiarem.
 */
#define TRIM_TO_RECORDS 40000

static const char* TAG = "DATA_SERVICE"; /**< Tag identyfikacyjny dla logów systemowych (ESP_LOG) */
static QueueHandle_t data_queue = NULL; /**< Uchwyt do głównej kolejki FreeRTOS buforującej paczki z czujników */
static volatile bool is_sending = false; /**< Flaga stanu wysyłki (volatile zapobiega nadmiernej optymalizacji przez kompilator) */
static SemaphoreHandle_t spiffs_mutex = NULL; /**< Semefor (Mutex) chroniący współdzielony dostęp do pamięci SPIFFS przed wyścigami wątków */


/**
 * @brief Zadanie FreeRTOS obsługujące ekstrakcję danych z kolejki i komunikację MQTT.
 * 
 * @details Działa w nieskończonej pętli. Funkcja wybudza się, gdy w kolejce pojawią się 
 * nowe dane. Formatuje strukturę C do postaci obiektu JSON, a następnie wysyła go 
 * za pomocą sprzętowego modemu. Jeśli negocjacja sieci zawiedzie, przerzuca odpowiedzialność 
 * na system plików SPIFFS.
 * 
 * @param[in] pvParameters Wskaźnik do parametrów zadania (nieużywany).
 */
static void data_sender_task(void* pvParameters) {
    sensor_data_t incoming_data;
    char json_buffer[768];
    bool mqtt_connected = false;

    while (1) {
        if (xQueuePeek(data_queue, &incoming_data, portMAX_DELAY)) {
            is_sending = true;
            if (!sim7070_wait_for_network()) {
                saveDataOffline(&incoming_data);
                xQueueReceive(data_queue, &incoming_data, 0);
                sim7070_mqtt_disconnect();
                mqtt_connected = false;
                is_sending = false;
                continue;
            }

            if (!mqtt_connected) {
                if (sim7070_mqtt_connect()) {
                    mqtt_connected = true;
                } else {
                    ESP_LOGW(TAG, "Błąd negocjacji MQTT. Ponowienie za 5s.");
                    saveDataOffline(&incoming_data);
                    xQueueReceive(data_queue, &incoming_data, 0);
                    is_sending = false;
                    continue;
                }
            }

            snprintf(
                json_buffer, sizeof(json_buffer),
                "{"
                "\"mcent\":{"
                "\"temp\":%.2f,"
                "\"hum\":%.2f,"
                "\"accelx\":%.2f,"
                "\"accely\":%.2f,"
                "\"accelz\":%.2f,"
                "\"presence\":%s,"
                "\"time\":%lu"
                "},"
                "\"scent\":{"
                "\"temp\":%.2f,"
                "\"hum\":%.2f,"
                "\"is_closed\":%s"
                "},"
                "\"location\":{"
                "\"latg\":%.6f,"
                "\"long\":%.6f"
                "},"
                "\"p1\":{"
                "\"p1x\":%.2f,"
                "\"p1y\":%.2f,"
                "\"p1z\":%.2f,"
                "\"p1mot\":%s"
                "},"
                "\"cell\":{"
                "\"mcc\":%u,"
                "\"mnc\":%u,"
                "\"tac\":%lu,"
                "\"cid\":%lu"
                "}"
                "}",
                incoming_data.temperature_main_central,
                incoming_data.humidity_main_central,
                incoming_data.accelx_main_central,
                incoming_data.accely_main_central,
                incoming_data.accelz_main_central,
                incoming_data.presence_main_central ? "true" : "false",
                (unsigned long)incoming_data.timestamp,
                incoming_data.temperature_secondary_central,
                incoming_data.humidity_secondary_central,
                incoming_data.is_closed_secondary_central ? "true" : "false",
                incoming_data.latitude, incoming_data.longitude,
                incoming_data.accelx_palette1, incoming_data.accely_palette1,
                incoming_data.accelz_palette1,
                incoming_data.motion_detected_p1 ? "true" : "false",
                incoming_data.cell_info.mcc, incoming_data.cell_info.mnc,
                incoming_data.cell_info.tac, incoming_data.cell_info.cid);

            ESP_LOGI(TAG, "Próba wysyłki: %s", json_buffer);

            if (sim7070_mqtt_send("dom/czujnik1", json_buffer)) {
                ESP_LOGI(TAG, "Pakiet wysłany pomyślnie.");
                xQueueReceive(data_queue, &incoming_data, 0);
            } else {
                ESP_LOGE(TAG, "Błąd wysyłania MQTT. Retransmisja");
                saveDataOffline(&incoming_data);
                xQueueReceive(data_queue, &incoming_data, 0);
                mqtt_connected = false;
            }

            sendBackupData();

            sim7070_mqtt_disconnect();
            mqtt_connected = false;
            is_sending = false;
        }
    }
}

void data_service_init(void) {
    data_queue = xQueueCreate(1, sizeof(sensor_data_t));
    spiffs_mutex = xSemaphoreCreateMutex();
    xTaskCreate(data_sender_task, "data_sender_task", 8192, NULL, 5, NULL);
}

bool data_service_push(sensor_data_t* data) {
    if (data_queue == NULL) return false;
    if (xQueueSend(data_queue, data, 0) == pdPASS) {
        is_sending = true;
        return true;
    } else {
        saveDataOffline(data);
        return false;
    }
}

bool data_service_is_busy(void) {
    if (data_queue == NULL) return false;
    return uxQueueMessagesWaiting(data_queue) > 0;
}

/**
 * @brief Chroni system plików przed przepełnieniem (Rotacja logów).
 * 
 * @details Mechanizm weryfikuje wielkość pliku "/data_backup.dat". Jeśli liczba zapisanych 
 * rekordów przekroczy zdefiniowany limit (MAX_BACKLOG_RECORDS), plik jest przycinany.
 * Funkcja usuwa najstarsze pomiary i zachowuje najnowsze (TRIM_TO_RECORDS). Dodatkowo 
 * implementuje warstwę autonaprawy w przypadku wykrycia uciętej/niepełnej struktury 
 * binarnej na skutek zaniku zasilania w trakcie zapisu na flash.
 */
static void trimBacklogIfNeeded() {
    if (!SPIFFS.exists("/data_backup.dat")) return;

    File dataFile = SPIFFS.open("/data_backup.dat", FILE_READ);
    if (!dataFile) return;

    size_t fileSize = dataFile.size();
    size_t currentRecords = fileSize / sizeof(sensor_data_t);
    
    bool needsTrim = (currentRecords >= MAX_BACKLOG_RECORDS);
    bool isCorrupted = (fileSize % sizeof(sensor_data_t) != 0); 

    if (!needsTrim && !isCorrupted) {
        dataFile.close();
        return;
    }

    if (isCorrupted) {
        ESP_LOGE(TAG, "Wykryto uszkodzony plik (niepełny zapis)! Rozpoczynam naprawę...");
    }

    size_t recordsToSkip = 0;
    if (needsTrim) {
        recordsToSkip = currentRecords - TRIM_TO_RECORDS;
    }

    dataFile.seek(recordsToSkip * sizeof(sensor_data_t));

    File tempFile = SPIFFS.open("/temp_trim.dat", FILE_WRITE);
    if (!tempFile) {
        dataFile.close();
        return;
    }

    uint8_t buf[sizeof(sensor_data_t)];
    while (dataFile.read(buf, sizeof(buf)) == sizeof(buf)) {
        tempFile.write(buf, sizeof(buf));
    }

    dataFile.close();
    tempFile.close();
    SPIFFS.remove("/data_backup.dat");
    SPIFFS.rename("/temp_trim.dat", "/data_backup.dat");

    if (needsTrim) {
        ESP_LOGW(TAG, "Backlog przycięty: usunięto najstarsze rekordy by chronic Flash!");
    }
    if (isCorrupted) {
        ESP_LOGI(TAG, "Naprawa ukończona: bezpiecznie usunięto niepełny zapis z końca pliku.");
    }
}

void saveDataOffline(sensor_data_t* data) {
    if (xSemaphoreTake(spiffs_mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
        trimBacklogIfNeeded();
        File dataFile = SPIFFS.open("/data_backup.dat", FILE_APPEND);
        if (dataFile) {
            size_t written = dataFile.write((uint8_t*)data, sizeof(sensor_data_t));
            if (written != sizeof(sensor_data_t)) {
                ESP_LOGE(TAG, "Niepełny zapis offline! Flash może być pełny!");
            }
            dataFile.close();
        }
        xSemaphoreGive(spiffs_mutex);
    }
}

bool sendBackupData() {
    if (xSemaphoreTake(spiffs_mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
        if (!SPIFFS.exists("/data_backup.dat")) {
            xSemaphoreGive(spiffs_mutex);
            return true;
        }
        File dataFile = SPIFFS.open("/data_backup.dat", FILE_READ);
        if (!dataFile || dataFile.size() == 0) {
            if (dataFile) dataFile.close();
            SPIFFS.remove("/data_backup.dat");
            xSemaphoreGive(spiffs_mutex);
            return true;
        }

        File tempFile = SPIFFS.open("/temp_backup.dat", FILE_WRITE);
        if (!tempFile) {
            dataFile.close();
            xSemaphoreGive(spiffs_mutex);
            return false;
        }

        sensor_data_t offlineData = {0};
        char json_buffer[768];
        int dataSent = 0;
        int dataNotSend = 0;
        bool networkFailed = false;

        const int MAX_BATCH_SIZE = 20; 
        int currentBatchCount = 0;

        while (dataFile.read((uint8_t*)&offlineData, sizeof(sensor_data_t)) == sizeof(sensor_data_t)) {
            if (currentBatchCount >= MAX_BATCH_SIZE) {
                dataNotSend++;
                tempFile.write((uint8_t*)&offlineData, sizeof(sensor_data_t));
                continue; 
            }

            
            if (!networkFailed) {
                snprintf(
                    json_buffer, sizeof(json_buffer),
                    "{\"mcent\":{\"temp\":%.2f,\"hum\":%.2f,\"accelx\":%.2f,"
                    "\"accely\":%.2f,\"accelz\":%.2f,\"presence\":%s,\"time\":%lu},"
                    "\"scent\":{\"temp\":%.2f,\"hum\":%.2f,\"is_closed\":%s},"
                    "\"location\":{\"latg\":%.6f,\"long\":%.6f},"
                    "\"p1\":{\"p1x\":%.2f,\"p1y\":%.2f,\"p1z\":%.2f,\"p1mot\":%s},"
                    "\"cell\":{\"mcc\":%u,\"mnc\":%u,\"tac\":%lu,\"cid\":%lu}}",
                    offlineData.temperature_main_central, offlineData.humidity_main_central,
                    offlineData.accelx_main_central, offlineData.accely_main_central, offlineData.accelz_main_central,
                    offlineData.presence_main_central ? "true" : "false", (unsigned long)offlineData.timestamp,
                    offlineData.temperature_secondary_central, offlineData.humidity_secondary_central,
                    offlineData.is_closed_secondary_central ? "true" : "false",
                    offlineData.latitude, offlineData.longitude,
                    offlineData.accelx_palette1, offlineData.accely_palette1, offlineData.accelz_palette1,
                    offlineData.motion_detected_p1 ? "true" : "false",
                    offlineData.cell_info.mcc, offlineData.cell_info.mnc,
                    offlineData.cell_info.tac, offlineData.cell_info.cid);

                if (sim7070_mqtt_send("dom/czujnik1", json_buffer)) {
                    dataSent++;
                    currentBatchCount++;
                } else {
                    networkFailed = true;
                    dataNotSend++;
                    tempFile.write((uint8_t*)&offlineData, sizeof(sensor_data_t));
                }
            } else {
                dataNotSend++;
                tempFile.write((uint8_t*)&offlineData, sizeof(sensor_data_t));
            }
        }

        dataFile.close();
        tempFile.close();
        SPIFFS.remove("/data_backup.dat");

        if (dataNotSend > 0) {
            SPIFFS.rename("/temp_backup.dat", "/data_backup.dat");
        } else {
            SPIFFS.remove("/temp_backup.dat");
        }
        ESP_LOGI(TAG, "Backlog: wysłano %d, pozostało %d", dataSent, dataNotSend);
        
        xSemaphoreGive(spiffs_mutex);
        return !networkFailed;
    }
    return false;
}

bool data_service_is_active(void) {
    return is_sending;
}
