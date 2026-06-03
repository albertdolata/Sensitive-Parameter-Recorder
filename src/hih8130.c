#include "hih8130.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HIH8130";

bool hih8130_read(hih8130_config_t *config, hih8130_data_t *data) {
    if (config == NULL || data == NULL) {
        return false;
    }

    i2c_cmd_handle_t cmd_mr = i2c_cmd_link_create();
    i2c_master_start(cmd_mr);
    i2c_master_write_byte(cmd_mr, (config->i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd_mr);
    
    esp_err_t err = i2c_master_cmd_begin(config->i2c_port, cmd_mr, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd_mr);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Błąd I2C (MR): %s", esp_err_to_name(err));
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rx_buf[4] = {0};
    i2c_cmd_handle_t cmd_df = i2c_cmd_link_create();
    i2c_master_start(cmd_df);
    i2c_master_write_byte(cmd_df, (config->i2c_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd_df, rx_buf, 3, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd_df, rx_buf + 3, I2C_MASTER_NACK); // Prawidłowe zakończenie ramki
    i2c_master_stop(cmd_df);
    
    err = i2c_master_cmd_begin(config->i2c_port, cmd_df, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd_df);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Błąd I2C (DF): %s", esp_err_to_name(err));
        return false;
    }

    uint8_t status = (rx_buf[0] >> 6) & 0x03;
    if (status != 0x00) {
        if (status == 0x01) ESP_LOGW(TAG, "Stale Data (t_conv przerwane)");
        else if (status == 0x02) ESP_LOGW(TAG, "Command Mode");
        else ESP_LOGE(TAG, "Błąd diagnostyczny układu (0x11)");
        return false;
    }
    uint16_t raw_rh = ((rx_buf[0] & 0x3F) << 8) | rx_buf[1];
    uint16_t raw_t = (rx_buf[2] << 6) | (rx_buf[3] >> 2);

    data->humidity = ((float)raw_rh / 16382.0f) * 100.0f;
    data->temperature = ((float)raw_t / 16382.0f) * 165.0f - 40.0f;

    return true;
}
