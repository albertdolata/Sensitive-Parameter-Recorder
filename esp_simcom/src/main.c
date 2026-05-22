#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sim7000_core.h"
#include "data_send_service.h" // Nowy serwis

#define SIM_RX_PIN GPIO_NUM_16
#define SIM_TX_PIN GPIO_NUM_17
#define SIM_PWR_PIN GPIO_NUM_4

static const char *TAG = "APP_MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Inicjalizacja systemu...");

    // 1. Inicjalizacja hardware'u
    if (!sim7000_init(SIM_RX_PIN, SIM_TX_PIN, SIM_PWR_PIN)) {
        ESP_LOGE(TAG, "Błąd inicjalizacji SIM7000!");
        return;
    }

    ESP_LOGI(TAG, "Oczekiwanie na sieć Orange...");
    if (!sim7000_wait_for_network()) {
        ESP_LOGE(TAG, "Nie udało się zalogować do sieci. Restartuję...");
        esp_restart();
    }

    // 3. Dopiero gdy mamy sieć, ruszamy z serwisem danych
    data_service_init();

    while (1) {
        sensor_data_t current_data = {0};

        current_data.cell_info = sim7000_get_network_params();

        if (current_data.cell_info.is_valid){

            current_data.temperature = 32.5f; 
            current_data.humidity = 45.0f;
            if (data_service_push(&current_data)) {
                ESP_LOGI(TAG, "Dane w kolejce.");
            }
        }
         else {
            ESP_LOGW(TAG, "Chwilowy brak parametrów CPSI, ponawiam...");
        }

        vTaskDelay(pdMS_TO_TICKS(30000)); 
    }
}