/**
 * @file time_manager.h
 * @brief Plik nagłówkowy zawierający deklaracje funkcji do synchronizacji czasu systemowego ESP32.
 */

#pragma once

#include <Arduino.h>
#include "../managers/GPSManager.h"

/**
 * @brief Aktualizuje wewnętrzny zegar czasu rzeczywistego (RTC) mikrokontrolera.
 *
 * @details Konwertuje przekazany znacznik czasu i ustawia globalny zegar systemowy ESP32. 
 * Posiadanie precyzyjnego, zsynchronizowanego czasu jest kluczowe dla prawidłowego 
 * stemplowania (timestamping) paczek pomiarowych, co ma szczególne znaczenie dla 
 * spójności bazy danych w przypadku archiwizacji offline (Backlog w pamięci Flash).
 *
 * @param[in] timestamp Znacznik czasu w formacie UNIX.
 */
void setESP32Time(uint32_t timestamp);

/**
 * @brief Weryfikuje i synchronizuje czas systemowy na podstawie odczytów satelitarnych GNSS.
 *
 * @details Funkcja odpytuje moduł GPS o najnowszy znacznik czasu. Jeśli urządzenie 
 * posiada poprawny fix satelitarny, pobrany czas jest porównywany z wartością 
 * last_gps_time. Aktualizacja sprzętowego zegara RTC następuje tylko w momencie 
 * wykrycia nowej, ważnej ramki. Mechanizm ten zapobiega zbędnemu, cyklicznemu 
 * nadpisywaniu czasu w głównej pętli programu.
 *
 * @param[in] gps Wskaźnik do obiektu zarządzającego sprzętowym odbiornikiem nawigacyjnym.
 * @param[in,out] last_gps_time Wskaźnik do zmiennej przechowującej ostatnio zsynchronizowany 
 * znacznik czasu. Wartość ta jest nadpisywana po każdej udanej aktualizacji zegara.
 */
void checkAndUpdateTime(GPSManager* gps, uint32_t* last_gps_time);