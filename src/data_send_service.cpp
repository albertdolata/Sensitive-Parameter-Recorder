#include "data_send_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

#include "sim7000_core.h"

static const char *TAG = "DATA_SERVICE";
static QueueHandle_t data_queue = NULL;


static void data_sender_task(void *pvParameters) {
    sensor_data_t incoming_data;
    char json_buffer[256];
    bool mqtt_connected = false;

    while (1) {
        if (xQueuePeek(data_queue, &incoming_data, portMAX_DELAY)) {
            
            if(!sim7000_wait_for_network()){
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            if(!mqtt_connected){
                if(sim7000_mqtt_connect(NULL)){
                    mqtt_connected = true;
                }else{
                    ESP_LOGW(TAG,"Błąd negocjacji MQTT. Ponowienie za 5s.");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    continue;
                }    
            }

            snprintf(json_buffer, sizeof(json_buffer),
            "{\"temp\":%.2f,\"hum\":%.2f,\"mcc\":%u,\"mnc\":%u,\"tac\":%lu,\"cid\":%lu}",
            incoming_data.temperature, incoming_data.humidity,
            incoming_data.cell_info.mcc, incoming_data.cell_info.mnc,
            incoming_data.cell_info.tac, incoming_data.cell_info.cid);

            ESP_LOGI(TAG, "Próba wysyłki: %s", json_buffer);

            if(sim7000_mqtt_send("dom/czujnik1",json_buffer)){
                ESP_LOGI(TAG,"Pakiet wysłany pomyślnie.");
                xQueueReceive(data_queue, &incoming_data, 0);
            }else{
                ESP_LOGE(TAG,"Błąd wysyłania MQTT. Retransmisja");
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

bool data_service_push(sensor_data_t *data) {
    if (data_queue == NULL) return false;
    return xQueueSend(data_queue, data, 0) == pdPASS;
}