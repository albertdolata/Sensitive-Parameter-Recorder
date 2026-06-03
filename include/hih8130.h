/**
 * @file hih8130.h
 * @brief Interfejs do komunikacji z czujnikiem wilgotności i temperatury HIH8130.
 * @details Zapewnia funkcję odczytują dane z czujnika.
 */

#pragma once

#include <driver/i2c.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#define HIH8130_DEFAULT_ADDR 0x27

typedef struct {
    float temperature; /**< Temperatura w stopniach Celsjusza. */
    float humidity;    /**< Wilgotność względna w procentach. */
} hih8130_data_t;

typedef struct {
    i2c_port_t i2c_port; /**< Numer portu I2C (np. I2C_NUM_0). */
    uint8_t i2c_addr;   /**< Adres I2C czujnika. */
} hih8130_config_t;

/**
 * @brief Pobiera dane z czujnika.
 * @attention Układ HIH wymaga wybudzenia i odczekania do 50ms.
 */
bool hih8130_read(hih8130_config_t *config, hih8130_data_t *data);


#ifdef __cplusplus
}
#endif