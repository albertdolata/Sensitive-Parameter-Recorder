/**
 * @file MainCentralSensorManager.h
 * @brief Plik nagłówkowy zawierający deklaracje klasy MainCentralSensorManager.
 */

#pragma once

#include <Arduino.h>

#include <string>

#include "../sensors/AccelSensor.h"
#include "../sensors/BLEPalletSensor.h"
#include "../sensors/BLESecondaryCentral.h"
#include "../sensors/PresenceSensor.h"
#include "../sensors/TempHumSensor.h"
#include "../simcom/data_send_service.h"
#include "BLESensorManager.h"

#define NUMBER_OF_SENSORS 5 /**< Całkowita liczba obsługiwanych czujników */

/**
 * @brief Klasa nadrzędna zarządzająca akwizycją danych ze wszystkich czujników.
 *
 * @details Działa jako główny agregator. Zarządza cyklem życia i odczytem
 * zarówno czujników lokalnych (podłączonych fizycznie do centrali),
 * jak i modułów oddalonych, komunikujących się za pośrednictwem interfejsu
 * Bluetooth Low Energy. Umożliwia zebranie wszystkich odczytów i spakowanie ich
 * do ujednoliconej struktury wysyłkowej.
 */
class MainCentralSensorManager {
   private:
    TempHumSensor mainEnvSensor;          /**< Lokalny czujnik temperatury i wilgotności (I2C) */
    PresenceSensor presenceSensor;        /**< Lokalny czujnik obecności / ruchu (GPIO) */
    AccelSensor mainAccelSensor;          /**< Lokalny akcelerometr centrali (I2C) */

    BLEPalletSensor palletSensor1;        /**< Zdalny czujnik paletowy (BLE) */
    BLESecondaryCentral secondaryCentral; /**< Zdalna centrala pomocnicza przy drzwiach naczepy (BLE) */
    BLESensorManager BLEManager;          /**< Wewnętrzny menedżer komunikacyji BLE */

    Sensor* sensors[NUMBER_OF_SENSORS];   /**< Tablica wskaźników bazowych do iteracji po czujnikach */

   public:
    /**
     * @brief Konstruktor klasy MainCentralSensorManager.
     *
     * @details Inicjalizuje obiekty składowe poszczególnych czujników,
     * przypisując im odpowiednie piny, adresy sprzętowe I2C oraz 
     * adresy MAC urządzeń zdalnych. Tworzy również tablicę 
     * wskaźników dla ułatwienia odczytów z wykorzystaniem pętli.
     *
     * @param presence_sensor_pin Numer pinu czujnika obecności.
     * @param accel_sensor_i2c_addr Adres I2C czujnika akcelerometru.
     * @param mac_pallet1 Adres MAC czujnika paletowego.
     * @param mac_secondary Adres MAC centrali pomocniczej.
     */
    MainCentralSensorManager(uint8_t presence_sensor_pin,
                             uint8_t accel_sensor_i2c_addr,
                             const std::string& mac_pallet1,
                             const std::string& mac_secondary);

    /** 
     * @brief Przeprowadza inicjalizację sprzętową wszystkich modułów.
     *
     * @details Wywołuje w pętli metodę startową dla każdego czujnika.
     * Konfiguruje magistrale (np. I2C) oraz uruchamia skaner BLE.
     *
     * @return true Jeśli wszystkie moduły poprawnie się zainicjalizowały.
     * @return false Jeśli przynajmniej jeden czujnik zgłosił błąd sprzętowy.
     */
    bool initializeAllSensors();

    /** 
     * @brief Wymusza asynchroniczny odczyt i odświeżenie danych we wszystkich modułach.
     *
     * @details Iteruje po tablicy czujników, wywołując ich wewnętrzne metody aktualizacji,
     * aby przygotować świeże dane do zrzutu.
     */
    void readAllSensorsData();

    /** 
     * @brief Zwraca wskaźnik do wewnętrznego menedżera BLE.
     *
     * @return BLESensorManager* Wskaźnik niezbędny do obsługi skanowania w głównej pętli.
     */
    BLESensorManager* getBLESensorManager();

    /** 
     * @brief Agreguje zebrane odczyty i wypełnia docelową strukturę wysyłkową.
     *
     * @details Przepisuje zbuforowane wartości ze wszystkich obiektów (lokalnych i BLE)
     * do jednej zunifikowanej ramki, która następnie trafia do kolejki MQTT lub pamięci SPIFFS.
     *
     * @param[out] data Wskaźnik na strukturę sensor_data_t, która zostanie nadpisana odczytami.
     */
    void fillSensorData(sensor_data_t* data);

    /** 
     * @brief Sprawdza, czy w aktualnym cyklu odebrano nowe ramki danych po BLE.
     *
     * @return true Jeśli sprzęt odebrał i sparsował nowy pakiet rozgłoszeniowy.
     */
    bool BleGotPackage();
};