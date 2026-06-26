#include "data_send_service.h"

#include <string.h>

#include "SPIFFS.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sim7070_core.h"

static const char* TAG = "DATA_SERVICE";
static QueueHandle_t data_queue = NULL;

static void data_sender_task(void* pvParameters) {
    sensor_data_t incoming_data;
    char json_buffer[768];
    bool mqtt_connected = false;

    while (1) {
        if (xQueuePeek(data_queue, &incoming_data, portMAX_DELAY)) {
            if (!sim7070_wait_for_network()) {
                saveDataOffline(&incoming_data);
                xQueueReceive(data_queue, &incoming_data, 0);
                sim7070_mqtt_disconnect();
                mqtt_connected = false;
                continue;
            }

            if (!mqtt_connected) {
                if (sim7070_mqtt_connect()) {
                    mqtt_connected = true;
                } else {
                    ESP_LOGW(TAG, "Błąd negocjacji MQTT. Ponowienie za 5s.");
                    saveDataOffline(&incoming_data);
                    xQueueReceive(data_queue, &incoming_data, 0);
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

            if (uxQueueMessagesWaiting(data_queue) == 0 && mqtt_connected) {
                ESP_LOGI(TAG,
                         "Kolejka pusta. Sprawdzam archiwum SPIFFS przed "
                         "oddaniem anteny...");
                sendBackupData();
                sim7070_mqtt_disconnect();
                mqtt_connected = false;
            }
        }
    }
}

void data_service_init(void) {
    data_queue = xQueueCreate(1, sizeof(sensor_data_t));
    xTaskCreate(data_sender_task, "data_sender_task", 8192, NULL, 5, NULL);
}

bool data_service_push(sensor_data_t* data) {
    if (data_queue == NULL) return false;
    if (xQueueSend(data_queue, data, 0) == pdPASS) {
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

void saveDataOffline(sensor_data_t* data) {
    File dataFile = SPIFFS.open("/data_backup.dat", FILE_APPEND);
    if (!dataFile) {
        return;
    } else {
        dataFile.write((uint8_t*)data, sizeof(sensor_data_t));
        dataFile.close();
    }
}

void sendBackupData() {
    if (!SPIFFS.exists("/data_backup.dat")) {
        return;
    }
    File dataFile = SPIFFS.open("/data_backup.dat", FILE_READ);
    if (!dataFile || dataFile.size() == 0) {
        SPIFFS.remove("/data_backup.dat");
        return;
    }

    File tempFile = SPIFFS.open("/temp_backup.dat", FILE_WRITE);
    if (!tempFile) {
        dataFile.close();
        return;
    }

    sensor_data_t offlineData = {0};
    char json_buffer[768];
    int dataNotSend = 0;
    bool networkFailed = false;

    while (dataFile.read((uint8_t*)&offlineData, sizeof(sensor_data_t)) ==
           sizeof(sensor_data_t)) {
        if (!networkFailed) {
            snprintf(
                json_buffer, sizeof(json_buffer),
                "{\"mcent\":{\"temp\":%.2f,\"hum\":%.2f,\"accelx\":%.2f,"
                "\"accely\":%.2f,\"accelz\":%.2f,\"presence\":%s,\"time\":%lu},"
                "\"scent\":{\"temp\":%.2f,\"hum\":%.2f,\"is_closed\":%s},"
                "\"location\":{\"latg\":%.6f,\"long\":%.6f},"
                "\"p1\":{\"p1x\":%.2f,\"p1y\":%.2f,\"p1z\":%.2f,\"p1mot\":%s},"
                "\"cell\":{\"mcc\":%u,\"mnc\":%u,\"tac\":%lu,\"cid\":%lu}}",
                offlineData.temperature_main_central,
                offlineData.humidity_main_central,
                offlineData.accelx_main_central,
                offlineData.accely_main_central,
                offlineData.accelz_main_central,
                offlineData.presence_main_central ? "true" : "false",
                (unsigned long)offlineData.timestamp,
                offlineData.temperature_secondary_central,
                offlineData.humidity_secondary_central,
                offlineData.is_closed_secondary_central ? "true" : "false",
                offlineData.latitude, offlineData.longitude,
                offlineData.accelx_palette1, offlineData.accely_palette1,
                offlineData.accelz_palette1,
                offlineData.motion_detected_p1 ? "true" : "false",
                offlineData.cell_info.mcc, offlineData.cell_info.mnc,
                offlineData.cell_info.tac, offlineData.cell_info.cid);

            if (sim7070_mqtt_send("dom/czujnik1", json_buffer)) {
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
}