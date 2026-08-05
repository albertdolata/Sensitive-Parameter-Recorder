/**
 * @file BLESecondaryCentral.h
 * @brief Plik nagłówkowy zawierający deklarację klasy BLESecondaryCentral.
 */

#pragma once

#include <Arduino.h>
#include "Sensor.h"

/**
 * @brief Klasa wirtualna reprezentująca zdalną centralę pomocniczą.
 *
 * @details Implementuje interfejs bazowy Sensor. Pełni rolę kontenera na dane
 * odbierane drogą radiową (BLE) z fizycznego modułu zamontowanego przy drzwiach naczepy.
 * Ponieważ dane spływają asynchronicznie (podczas skanowania rozgłoszeń BLE),
 * klasa ta nie odpytuje sprzętu w klasyczny sposób, lecz jest na bieżąco
 * aktualizowana z zewnątrz za pomocą metody updateDataFromBLE.
 */
class BLESecondaryCentral : public Sensor {
   private:
    float temp; /**< Ostatnia odczytana wartość temperatury w stopniach Celsjusza */
    float hum; /**< Ostatnia odczytana wartość wilgotności względnej w procentach */
    bool isClosed; /**< Flaga statusu kontaktronu (true = drzwi zamknięte, false = otwarte) */

   public:
    BLESecondaryCentral();

    /** 
     * @brief Inicjalizuje wirtualny czujnik centrali pomocniczej.
     *
     * @details Ponieważ jest to klasa wirtualna dla modułu BLE, który konfiguruje się
     * na poziomie menedżera radia, ta funkcja nie wykonuje sprzętowych operacji I2C/SPI.
     *
     * @return Zawsze zwraca true.
     */
    bool initialize() override;

    /** 
     * @brief Atrapa funkcji odczytu (wymóg polimorfizmu interfejsu Sensor).
     *
     * @details W tej klasie metoda pozostaje pusta, ponieważ 
     * rzeczywisty odczyt jest napędzany zdarzeniami ze skanera BLE.
     */
    void readData() override;

    /** 
     * @brief Zwraca zapamiętaną wartość temperatury.
     * @return Temperatura z tylnej części naczepy w stopniach Celsjusza.
     */
    float getTemp() const;

    /** 
     * @brief Zwraca zapamiętaną wartość wilgotności.
     * @return Wilgotność względna z tylnej części naczepy w procentach.
     */
    float getHumidity() const;

    /** 
     * @brief Zwraca zapamiętany stan drzwi naczepy (odczyt z kontaktronu).
     * @return true jeśli drzwi są zamknięte, false jeśli otwarto przestrzeń ładunkową.
     */
    bool getIsClosed() const;

    /** 
     * @brief Asynchronicznie aktualizuje stan obiektu nowymi danymi z paczki rozgłoszeniowej BLE.
     *
     * @details Metoda jest wywoływana przez BLESensorManager po pomyślnym odebraniu 
     * danych z adresu MAC przypisanego do centrali pomocniczej. Konwertuje przysłane 
     * wartości całkowite na docelowe wartości zmiennoprzecinkowe.
     *
     * @param[in] temp Surowy odczyt temperatury (w setnych częściach stopnia Celsjusza).
     * @param[in] hum Surowy odczyt wilgotności (w setnych częściach procenta).
     * @param[in] isClosed Stan logiczny z wejścia cyfrowego kontaktronu (1 = zamknięte, 0 = otwarte).
     */
    void updateDataFromBLE(int16_t temp, uint16_t hum, uint8_t isClosed);
};