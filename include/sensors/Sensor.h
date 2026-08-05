/**
 * @file Sensor.h
 * @brief Plik nagłówkowy zawierający deklarację klasy bazowej Sensor.
 */
#pragma once

/**
 * @brief Abstrakcyjny interfejs dla wszystkich czujników w systemie.
 *
 * @details Wymusza jednolity kontrakt dla wszystkich modułów pomiarowych,
 * zarówno lokalnych (I2C, GPIO), jak i zdalnych (BLE). Dzięki temu główny
 * agregator (MainCentralSensorManager) może przechowywać wskaźniki w jednej
 * polimorficznej tablicy i zarządzać cyklem życia różnorodnych czujników
 * za pomocą tej samej, uniwersalnej logiki.
 */
class Sensor {
   public:
    Sensor() {}

    virtual ~Sensor() {}

    /**
     * @brief Metoda czysto wirtualna wymuszająca implementację inicjalizacji.
     *
     * @details Musi zostać nadpisana (override) w każdej klasie dziedziczącej.
     * Służy do sprzętowej (np. magistrala I2C, piny GPIO) lub programowej
     * konfiguracji danego detektora przed rozpoczęciem cyklu pomiarowego.
     *
     * @return true Jeśli czujnik pomyślnie przeszedł sekwencję rozruchową.
     * @return false W przypadku błędu sprzętowego, braku komunikacji lub złej
     * konfiguracji.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Metoda czysto wirtualna wymuszająca implementację pobierania
     * próbek.
     *
     * @details Musi zostać nadpisana (override) w każdej klasie dziedziczącej.
     * Definiuje sposób odpytywania rejestrów sprzętowych (I2C/SPI/GPIO)
     * lub aktualizacji zbuforowanych zmiennych środowiskowych.
     */
    virtual void readData() = 0;
};