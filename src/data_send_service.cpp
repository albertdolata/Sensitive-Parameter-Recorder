#include "data_send_service.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sim7000_core.h"

static const char* TAG = "DATA_SERVICE";
static QueueHandle_t data_queue = NULL;

static void data_sender_task(void* pvParameters) {
    sensor_data_t incoming_data;
    char json_buffer[512];
    bool mqtt_connected = false;

    while (1) {
        if (xQueuePeek(data_queue, &incoming_data, portMAX_DELAY)) {
            if (!sim7000_wait_for_network()) {
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            if (!mqtt_connected) {
                if (sim7000_mqtt_connect()) {
                    mqtt_connected = true;
                } else {
                    ESP_LOGW(TAG, "Błąd negocjacji MQTT. Ponowienie za 5s.");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    continue;
                }
            }

            snprintf(json_buffer, sizeof(json_buffer),
                     "{"
                     "\"mcent\":{"
                     "\"temp\":%.2f,"
                     "\"hum\":%.2f,"
                     "\"shock\":%.2f,"
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
                     "\"shock\":%.2f"
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
                     incoming_data.shock_level_main_central,
                     incoming_data.presence_main_central ? "true" : "false",
                     (unsigned long)incoming_data.timestamp,
                     incoming_data.temperature_secondary_central,
                     incoming_data.humidity_secondary_central,
                     incoming_data.is_closed_secondary_central ? "true" : "false",
                     incoming_data.latitude, incoming_data.longitude,
                     incoming_data.shock_level_palette1,
                     incoming_data.cell_info.mcc, incoming_data.cell_info.mnc,
                     incoming_data.cell_info.tac, incoming_data.cell_info.cid);

            ESP_LOGI(TAG, "Próba wysyłki: %s", json_buffer);

            if (sim7000_mqtt_send("dom/czujnik1", json_buffer)) {
                ESP_LOGI(TAG, "Pakiet wysłany pomyślnie.");
                xQueueReceive(data_queue, &incoming_data, 0);
            } else {
                ESP_LOGE(TAG, "Błąd wysyłania MQTT. Retransmisja");
                mqtt_connected = false;
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
    }
}
void data_service_init(void) {
    data_queue = xQueueCreate(10, sizeof(sensor_data_t));
    xTaskCreate(data_sender_task, "data_sender_task", 4096, NULL, 5, NULL);
}

bool data_service_push(sensor_data_t* data) {
    if (data_queue == NULL) return false;
    return xQueueSend(data_queue, data, 0) == pdPASS;
}