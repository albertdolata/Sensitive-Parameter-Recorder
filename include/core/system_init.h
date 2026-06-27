#pragma once

#include <Arduino.h>

void SPIFFSinit();

void SIMComInit(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin);