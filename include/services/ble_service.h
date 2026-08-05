/**
 * @file ble_service.h
 * @brief Plik nagłówkowy zawierający deklaracje funkcji i zadań FreeRTOS do obsługi skanowania BLE.
 */

#pragma once

#include <Arduino.h>
#include "../managers/BLESensorManager.h"

/**
 * @brief Inicjalizuje stos Bluetooth Low Energy w trybie skanera.
 *
 * @details Uruchamia sprzętowy interfejs radiowy ESP32 i przypisuje przekazany obiekt 
 * menedżera jako funkcję zwrotną dla asynchronicznych zdarzeń odnajdywania urządzeń.
 * Konfiguruje podstawowe parametry radia, takie jak czas trwania okna skanowania 
 * oraz interwał.
 *
 * @param[in] bleManager Wskaźnik do instancji BLESensorManager obsługującej logikę parsowania rozgłoszeń.
 */
void BLEInit(BLESensorManager* bleManager);

/**
 * @brief Główne zadanie FreeRTOS odpowiedzialne za cykliczny nasłuch eteru.
 *
 * @details Funkcja działa w nieskończonej pętli, cyklicznie uruchamiając sprzętowe 
 * skanowanie rozgłoszeń z czujników paletowych i centrali pomocniczej.
 * Zadanie to jest sztywno przypisywane do rdzenia zerowego (Core 0) mikrokontrolera 
 * za pomocą mechanizmu xTaskCreatePinnedToCore. Dzięki takiemu podziałowi, wymagająca 
 * sprzętowo obsługa radia BLE działa ciągle w tle, nie blokując i nie zakłócając 
 * głównej logiki programu, operującej na rdzeniu aplikacyjnym (Core 1).
 * Kluczowym elementem zadania jest również czyszczenie bufora wyników po każdym pełnym skanie, 
 * co zapobiega wyciekom pamięci w stosie BLE mikrokontrolera.
 *
 * @param[in] parameter Wskaźnik do parametru przekazanego przy tworzeniu zadania (nieużywany).
 */
void bleScanTask(void* parameter);