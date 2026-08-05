/**
 * @file BLEPalletSensor.h
 * @brief Plik nagłówkowy zawierający deklarację klasy BLEPalletSensor.
 */

#pragma once

#include <Arduino.h>
#include "Sensor.h"

/**
 * @brief Klasa wirtualna reprezentująca zdalny akcelerometr na palecie.
 *
 * @details Implementuje interfejs bazowy Sensor. Pełni rolę kontenera na dane 
 * odbierane drogą radiową (BLE) z fizycznego modułu zamontowanego na ładunku. 
 * Ponieważ dane spływają asynchronicznie (podczas skanowania rozgłoszeń BLE), 
 * klasa ta nie odpytuje sprzętu w klasyczny sposób, lecz jest na bieżąco 
 * aktualizowana z zewnątrz za pomocą metody updateDataFromBLE.
 */
class BLEPalletSensor : public Sensor {
   private:
    float axisX; /**< Ostatnia odczytana wartość przyspieszenia wzdłuż osi X */
    float axisY; /**< Ostatnia odczytana wartość przyspieszenia wzdłuż osi Y */
    float axisZ; /**< Ostatnia odczytana wartość przyspieszenia wzdłuż osi Z */
    bool motionDetected; /**< Flaga wskazująca, czy wykryto ruch palety */

   public:
    BLEPalletSensor();
    
    /** 
     * @brief Inicjalizuje wirtualny czujnik.
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
     * @brief Zwraca zapamiętaną wartość przyspieszenia w osi X.
     * @return Przyspieszenie wzdłuż osi X.
     */
    float getAxisX() const;

    /** 
     * @brief Zwraca zapamiętaną wartość przyspieszenia w osi Y.
     * @return Przyspieszenie wzdłuż osi Y.
     */
    float getAxisY() const;

    /** 
     * @brief Zwraca zapamiętaną wartość przyspieszenia w osi Z.
     * @return Przyspieszenie wzdłuż osi Z.
     */
    float getAxisZ() const;

    /** 
     * @brief Sprawdza, czy zdalny moduł zaraportował poruszenie ładunku.
     * @return true Jeśli wykryto ruch palety, w przeciwnym razie false.
     */
    bool isMotionDetected() const;

    /** 
     * @brief Asynchronicznie aktualizuje stan obiektu nowymi danymi z paczki rozgłoszeniowej BLE.
     *
     * @details Metoda jest wywoływana przez BLESensorManager po pomyślnym odebraniu 
     * i rozszyfrowaniu danych z adresu MAC przypisanego do tej konkretnej palety.
     * Konwertuje wartości przyspieszeń z formatu 16-bitowego na wartości 
     * zmiennoprzecinkowe (float) określające fizyczne przeciążenie.
     *
     * @param[in] x Surowy odczyt przyspieszenia w osi X (16-bitowa wartość).
     * @param[in] y Surowy odczyt przyspieszenia w osi Y (16-bitowa wartość).
     * @param[in] z Surowy odczyt przyspieszenia w osi Z (16-bitowa wartość).
     * @param[in] motion Flaga z rejestru alarmowego modułu zdalnego (1 = ruch, 0 = spokój).
     */
    void updateDataFromBLE(int16_t x, int16_t y, int16_t z, uint8_t motion);
};