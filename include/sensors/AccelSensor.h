/**
 * @file AccelSensor.h
 * @brief Plik nagłówkowy zawierający deklarację klasy AccelSensor.
 */

#pragma once

#include <Adafruit_LIS3DH.h>
#include "Sensor.h"

/** 
 * @brief Klasa obsługująca sprzętowy akcelerometr (LIS3DH) podłączony lokalnie.
 *
 * @details Implementuje interfejs bazowy Sensor. Służy do pomiaru przyspieszeń 
 * trójosiowych w głównej centrali, co pozwala na detekcję wstrząsów, wibracji 
 * oraz zmian położenia naczepy. Wykorzystuje magistralę I2C do komunikacji.
 */
class AccelSensor : public Sensor {
   private:
    Adafruit_LIS3DH lis3dh; /**< Obiekt biblioteki Adafruit do komunikacji z układem LIS3DH */
    uint8_t i2c_address;    /**< Adres fizyczny urządzenia na magistrali I2C */
    float axisX;            /**< Ostatnia odczytana wartość przyspieszenia wzdłuż osi X */
    float axisY;            /**< Ostatnia odczytana wartość przyspieszenia wzdłuż osi Y */
    float axisZ;            /**< Ostatnia odczytana wartość przyspieszenia wzdłuż osi Z */

   public:
   /** 
     * @brief Konstruktor sprzętowego czujnika przeciążeń.
     *
     * @param[in] addr Adres I2C modułu akcelerometru.
     */
    AccelSensor(uint8_t addr);

    /** 
     * @brief Uruchamia układ LIS3DH i konfiguruje jego parametry początkowe.
     *
     * @details Inicjalizuje połączenie I2C, weryfikuje identyfikator sprzętowy (WhoAmI) 
     * oraz ustawia podstawowy zakres pomiarowy i częstotliwość próbkowania.
     * Przesłania metodę wirtualną z klasy bazowej Sensor.
     *
     * @return true Jeśli czujnik został pomyślnie wykryty i skonfigurowany.
     */
    bool initialize() override;

    /** 
     * @brief Pobiera najnowsze próbki pomiarowe z rejestrów układu LIS3DH.
     *
     * @details Wywołuje odczyt sprzętowy i aktualizuje wewnętrzne zmienne axisX, 
     * axisY oraz axisZ. Przesłania metodę wirtualną z klasy bazowej Sensor.
     */
    void readData() override;

    /** 
     * @brief Zwraca zapamiętaną wartość przyspieszenia w osi X.
     * @return Aktualna wartość przyspieszenia wzdłuż osi X.
     */
    float getAxisX() const;

    /** 
     * @brief Zwraca zapamiętaną wartość przyspieszenia w osi Y.
     * @return Aktualna wartość przyspieszenia wzdłuż osi Y.
     */
    float getAxisY() const;

    /** 
     * @brief Zwraca zapamiętaną wartość przyspieszenia w osi Z.
     * @return Aktualna wartość przyspieszenia wzdłuż osi Z.
     */
    float getAxisZ() const;
};