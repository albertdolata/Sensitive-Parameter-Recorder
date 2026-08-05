/**
 * @file PresenceSensor.h
 * @brief Plik nagłówkowy zawierający deklarację klasy PresenceSensor.
 */

#pragma once

#include <Arduino.h>
#include "Sensor.h"

/**
 * @brief Klasa obsługująca czujnik obecności wewnątrz naczepy.
 *
 * @details Implementuje interfejs bazowy Sensor. Odpowiada za monitorowanie przestrzeni 
 * ładunkowej pod kątem nieautoryzowanego dostępu. Klasa zarządza odczytem stanu 
 * logicznego z wejścia cyfrowego GPIO, do którego podpięty jest fizyczny czujnik.
 */
class PresenceSensor : public Sensor {
   private:
    bool isSomeOneHere; /**< Flaga wskazująca, czy wykryto obecność (true = obecność, false = brak obecności) */
    uint8_t GPIO_pin; /**< Numer pinu GPIO, do którego podłączony jest czujnik obecności */

   public:
   /** 
     * @brief Konstruktor sprzętowego czujnika obecności.
     *
     * @param[in] pin Numer pinu GPIO mikrokontrolera, do którego podłączono wyjście sygnałowe czujnika.
     */
    PresenceSensor(uint8_t pin);

    /** 
     * @brief Konfiguruje mikrokontroler do współpracy z detektorem.
     *
     * @details Ustawia tryb przypisanego pinu GPIO na wejście cyfrowe (pinMode). 
     * Przesłania metodę wirtualną z klasy bazowej Sensor.
     *
     * @return Zawsze zwraca true.
     */
    bool initialize() override;

    /** 
     * @brief Wykonuje sprzętowy odczyt stanu wejścia.
     *
     * @details Odczytuje fizyczny stan napięcia na pinie GPIO (za pomocą digitalRead) 
     * i buforuje wynik logiczny w wewnętrznej zmiennej isSomeOneHere.
     */
    void readData() override;

    /** 
     * @brief Zwraca zapamiętany stan detekcji intruza.
     *
     * @return true Jeśli w ostatnim cyklu pomiarowym czujnik zaraportował obecność.
     */
    bool getPresence() const;
};
