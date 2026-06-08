#ifndef MMWAVE_SENSOR_H_
#define MMWAVE_SENSOR_H_

#include <stdbool.h>
#include "driver/uart.h"
#include "driver/gpio.h"

#define MMWAVE_UART_NUM      UART_NUM_1
#define MMWAVE_RX_PIN        GPIO_NUM_18
#define MMWAVE_TX_PIN        GPIO_NUM_19
#define MMWAVE_OUT_PIN       GPIO_NUM_5

#define MMWAVE_BUF_SIZE      256

typedef struct {
    bool target_present;
    float distance_meters;
} mmwave_data_t;

/**
 * @brief Inicjalizuje peryferia UART oraz GPIO dla sensora mmWave.
 * @return esp_err_t ESP_OK w przypadku sukcesu.
 */
esp_err_t mmwave_init(void);

/**
 * @brief Odczytuje stan pinu OUT (Szybka ścieżka binarna).
 * @return true jeśli wykryto obiekt, false w przeciwnym wypadku.
 */
bool mmwave_get_presence_discrete(void);

/**
 * @brief Parsuje strumień UART i aktualizuje strukturę danych.
 * @param data Wskaźnik na strukturę wyjściową.
 */
bool mmwave_read_data(mmwave_data_t *data);

#endif /* MMWAVE_SENSOR_H_ */