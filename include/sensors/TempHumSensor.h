/**
 * @file TempHumSensor.h
 * @brief Plik nagłówkowy zawierający deklarację klasy TempHumSensor.
 */

#pragma once

#include <Adafruit_Si7021.h>
#include "Sensor.h"

/**
 * @brief Klasa obsługująca lokalny czujnik temperatury i wilgotności (Si7021).
 *
 * @details Dziedziczy po interfejsie bazowym Sensor. Odpowiada za monitorowanie 
 * warunków środowiskowych panujących bezpośrednio w otoczeniu centrali głównej 
 * (przednia część naczepy). Wykorzystuje magistralę I2C do komunikacji z fizycznym układem.
 */
class TempHumSensor : public Sensor {
   private:
    Adafruit_Si7021 adafruit_si7021; /**< Obiekt biblioteki Adafruit do sprzętowej obsługi układu Si7021 */
    float temperature;               /**< Ostatnia odczytana wartość temperatury w stopniach Celsjusza */
    float humidity;                  /**< Ostatnia odczytana wartość wilgotności względnej w procentach */

   public:
    TempHumSensor();

    /**
     * @brief Inicjalizuje układ Si7021 i nawiązuje komunikację po I2C.
     *
     * @details Konfiguruje sprzętową magistralę I2C i weryfikuje dostępność czujnika.
     * Przesłania metodę wirtualną z klasy bazowej Sensor.
     *
     * @return true Jeśli czujnik został pomyślnie wykryty i zainicjalizowany.
     */
    bool initialize() override;

    /**
     * @brief Pobiera najnowsze wskazania pomiarowe z układu Si7021.
     *
     * @details Wykonuje odczyt sprzętowy z użyciem zewnętrznej biblioteki, a następnie 
     * zapisuje aktualne wartości w wewnętrznych zmiennych temperature i humidity.
     * Przesłania metodę wirtualną z klasy Sensor.
     */
    void readData() override;

    /**
     * @brief Zwraca ostatnio odczytaną wartość temperatury.
     *
     * @return Ostatnia zmierzona temperatura w stopniach Celsjusza.
     */
    float getTemperature() const;

    /**
     * @brief Zwraca ostatnio odczytaną wartość wilgotności.
     *
     * @return Ostatnia zmierzona wilgotność w procentach.
     */
    float getHumidity() const;
};