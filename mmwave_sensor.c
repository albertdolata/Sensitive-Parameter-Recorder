#include "mmwave_sensor.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *MMW_TAG = "MMWAVE_DRV";

esp_err_t mmwave_init(void) {
    // 1. Konfiguracja UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t ret = uart_param_config(MMWAVE_UART_NUM, &uart_config);
    if (ret != ESP_OK) return ret;

    ret = uart_set_pin(MMWAVE_UART_NUM, MMWAVE_TX_PIN, MMWAVE_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) return ret;

    ret = uart_driver_install(MMWAVE_UART_NUM, MMWAVE_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) return ret;

    // 2. Konfiguracja pinu cyfrowego OUT
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE, 
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << MMWAVE_OUT_PIN),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    
    return gpio_config(&io_conf);
}

bool mmwave_get_presence_discrete(void) {
    return gpio_get_level(MMWAVE_OUT_PIN) == 1;
}

void mmwave_read_data(mmwave_data_t *data) {
    static char rx_stream_buf[MMWAVE_BUF_SIZE];
    size_t available_bytes = 0;
    
    // Sprawdzenie dostępności danych w buforze
    uart_get_buffered_data_len(MMWAVE_UART_NUM, &available_bytes);
    if (available_bytes == 0) {
        return; 
    }

    // JEDYNE wywołanie uart_read_bytes w całej aplikacji dla tego portu UART
    int read_len = uart_read_bytes(MMWAVE_UART_NUM, (uint8_t*)rx_stream_buf, MMWAVE_BUF_SIZE - 1, pdMS_TO_TICKS(10));
    if (read_len <= 0) return;

    rx_stream_buf[read_len] = '\0'; 

    // Diagnostyczny zrzut przeniesiony tutaj (opcjonalny, do weryfikacji)
    // ESP_LOGI(MMW_TAG, "RAW INTERNAL: %s", rx_stream_buf);

    // 1. Analiza statusu obecności
    if (strstr(rx_stream_buf, "ON") != NULL) {
        data->target_present = true;
    } else if (strstr(rx_stream_buf, "OFF") != NULL) {
        data->target_present = false;
        data->distance_meters = 0.0f;
        return;
    }

    // 2. Analiza odległości - wyszukiwanie klucza "Range "
    char *range_ptr = strstr(rx_stream_buf, "Range ");
    if (range_ptr != NULL) {
        int range_cm = atoi(range_ptr + 6);
        data->distance_meters = (float)range_cm / 100.0f;
    }
}