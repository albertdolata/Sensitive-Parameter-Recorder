/**
 * @file data_send_service.h
 * @brief Usługa asynchronicznej wysyłki danych z sensorów oraz obsługi pamięci offline.
 * 
 * @details Moduł odpowiada za kolejkowanie (FreeRTOS) odczytów z czujników 
 * i przesyłanie ich na serwer MQTT przez modem SIM7070. W przypadku utraty 
 * połączenia z siecią, dane są bezpiecznie zrzucane do pamięci flash (SPIFFS), 
 * tworząc lokalny backlog, który jest automatycznie wysyłany po odzyskaniu zasięgu.
 */

#pragma once

#include "sim7070_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct sensor_data_t
 * @brief Ujednolicona struktura danych pomiarowych ze wszystkich punktów naczepy.
 * 
 * @note Aby dodać nowy czujnik:
 * 1. Dodaj odpowiednie pole do tej struktury.
 * 2. Zaktualizuj formatowanie JSON w pliku data_send_service.cpp (wewnątrz data_sender_task).
 * 3. Zwiększ rozmiar bufora json_buffer, jeśli to konieczne.
 */
typedef struct {
// --- Dane z sensorów centrali głównej (przód naczepy) ---
    float temperature_main_central; /**< Temperatura w stopniach Celsjusza */
    float humidity_main_central;    /**< Wilgotność względna w procentach */
    float accelx_main_central;      /**< Przyspieszenie w osi X (akcelerometr centrali) */
    float accely_main_central;      /**< Przyspieszenie w osi Y (akcelerometr centrali) */
    float accelz_main_central;      /**< Przyspieszenie w osi Z (akcelerometr centrali) */
    bool presence_main_central;     /**< Stan detekcji ruchu (true = intruz w naczepie) */
    
    // --- Dane z centrali pomocniczej (drzwi naczepy) ---
    float temperature_secondary_central; /**< Temperatura w stopniach Celsjusza (tył) */
    float humidity_secondary_central;    /**< Wilgotność względna w procentach (tył) */
    bool is_closed_secondary_central;    /**< Stan drzwi (true = zamknięte, false = otwarte) */
    
    // --- Dane lokalizacyjne i sieciowe (GNSS / LBS) ---
    double latitude;        /**< Szerokość geograficzna ze sprzętowego odbiornika GNSS */
    double longitude;       /**< Długość geograficzna ze sprzętowego odbiornika GNSS */
    cell_info_t cell_info;  /**< Metadane stacji bazowej GSM (MNC, MCC, TAC, CID) do weryfikacji LBS */
    
    // --- Dane ze zdalnych czujników na paletach (BLE) ---
    float accelx_palette1;      /**< Przyspieszenie w osi X z czujnika na palecie 1 */
    float accely_palette1;      /**< Przyspieszenie w osi Y z czujnika na palecie 1 */
    float accelz_palette1;      /**< Przyspieszenie w osi Z z czujnika na palecie 1 */
    bool motion_detected_p1;    /**< Flaga poruszenia ładunku (true = wstrząs/ruch) */
    
    uint32_t timestamp;         /**< Znacznik czasu pomiaru w formacie UNIX Epoch (UTC) */
} sensor_data_t;

/**
 * @brief Uruchamia system kolejkowania i inicjalizuje niezbędne zasoby.
 * 
 * @details Tworzy kolejkę FreeRTOS dla struktur sensor_data_t, alokuje semafor (Mutex) 
 * chroniący dostęp do systemu plików SPIFFS oraz uruchamia główne zadanie wysyłkowe (data_sender_task).
 */
void data_service_init(void);

/**
 * @brief Wrzuca nową paczkę danych do kolejki transmisyjnej.
 * 
 * @details Funkcja jest całkowicie nieblokująca (timeout = 0). Pozwala to głównej 
 * pętli programu na natychmiastowy powrót do obsługi czujników. Jeśli główna kolejka 
 * jest pełna, dane zostaną automatycznie przekierowane do pamięci offline (SPIFFS).
 * 
 * @param[in] data Wskaźnik na wypełnioną strukturę pomiarową.
 * @return true Jeśli dane pomyślnie trafiły do kolejki FreeRTOS.
 * @return false Jeśli kolejka była pełna i wykonano zapis awaryjny do SPIFFS.
 */
bool data_service_push(sensor_data_t* data);

/**
 * @brief Sprawdza, czy w kolejce oczekują ramki danych na wysłanie.
 * @return true Jeśli w kolejce znajduje się co najmniej jeden element.
 */
bool data_service_is_busy(void);

/**
 * @brief Wykonuje awaryjny zapis paczki danych do pamięci Flash.
 * 
 * @details Korzysta z mechanizmu Mutex, aby bezpiecznie dopisać strukturę 
 * binarną na koniec pliku bufora w pamięci SPIFFS. Posiada mechanizm 
 * zapobiegający przepełnieniu pamięci (Trim).
 * 
 * @param[in] data Wskaźnik na strukturę, która ma zostać zarchiwizowana.
 */
void saveDataOffline(sensor_data_t* data);

/**
 * @brief Podejmuje próbę opróżnienia archiwum offline (Backlog).
 * 
 * @details Odczytuje zapisane wcześniej rekordy z pamięci SPIFFS i próbuje 
 * wysłać je jeden po drugim na serwer MQTT. W przypadku ponownego zerwania 
 * połączenia z siecią w trakcie wysyłania, niewysłana reszta wraca bezpiecznie na dysk.
 * 
 * @return true Jeśli cały plik backlogu został pomyślnie wysłany i usunięty.
 * @return false Jeśli wysyłanie przerwało się z powodu błędu sieci GSM.
 */
bool sendBackupData();

/**
 * @brief Sprawdza bieżący status działania usługi wysyłkowej.
 * @return true Jeśli task transmisyjny jest w trakcie procesowania danych.
 */
bool data_service_is_active(void);

#ifdef __cplusplus
}
#endif