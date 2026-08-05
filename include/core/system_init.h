/**
 * @file system_init.h
 * @brief Plik nagłówkowy zawierający funkcje inicjalizujące dla zarządzania
 * pamięcią SPIFFS oraz modemu SIMCom.
 *
 * @details Plik zawiera deklaracje funkcji SPIFFSinit() oraz SIMComInit()
 * wykorzystywanych w pętli setup() programu.
 */

#pragma once

#include <Arduino.h>
/**
 * @brief Montuje partycję na pliki w pamięci FLASH.
 *
 * @details Inicjalizuje za pomocą biblioteki SPIFFS partycję dla plików, w 
 * których zapisywane będą dane offline. Jeżeli pamięć jest uszkodzona lub 
 * niezgodna z flagą z platformio.ini, formatuje odpowiednio pamięć FLASH.
 * W przypadku niepowodzenia, funkcja wywołuje twardy restart mikrokontrolera.
 */
void SPIFFSinit();

/**
 * @brief Nadzoruje proces uruchamiania sprzętowego i logowania modemu do sieci.
 *
 * @details Funkcja pełni rolę watchdog'a dla procedur startowych. 
 * Wywołuje właściwą funkcję inicjalizującą modem (sim7070_init), a następnie 
 * funkcję oczekującą na rejestrację operatora (sim7070_wait_for_network). 
 * Jeśli którykolwiek z tych etapów zawiedzie, funkcja wymusza twardy restart 
 * całego mikrokontrolera ESP32, aby spróbować ponownie.
 *
 * @param[in] rx_pin Pin RX ESP32 (połączony z TX modemu).
 * @param[in] tx_pin Pin TX ESP32 (połączony z RX modemu).
 * @param[in] pwr_pin Pin sterujący kluczem tranzystorowym zasilania modemu SIMCom.
 */
void SIMComInit(gpio_num_t rx_pin, gpio_num_t tx_pin, gpio_num_t pwr_pin);