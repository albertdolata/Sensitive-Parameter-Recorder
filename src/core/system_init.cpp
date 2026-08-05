/**
 * @file system_init.cpp
 * @brief Implementacja funkcji inicjalizujących system sprzętowy (SPIFFS i SIMCom).
 */
#include "../include/core/system_init.h"
#include "../include/simcom/sim7070_core.h"
#include "SPIFFS.h"

void SPIFFSinit() {
    if (!SPIFFS.begin(true)) {
        ESP.restart();
    }
}

void SIMComInit(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin) {
    if (!sim7070_init(rx_pin, tx_pin, pwr_pin)) {
        ESP.restart();
    }

    if (!sim7070_wait_for_network()) {
        ESP.restart();
    }
}
