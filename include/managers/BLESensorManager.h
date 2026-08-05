/**
 * @file BLESensorManager.cpp
 * @brief Plik implementacyjny zawierający definicje metod klasy BLESensorManager.
 */

#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>

#include "../sensors/BLEPalletSensor.h"
#include "../sensors/BLESecondaryCentral.h"
#include "../sensors/Sensor.h"

/**
 * @brief  Struktura reprezentująca dane przesyłane przez czujniki BLE.
 *
 * @details Struktura jest używana do przechowywania danych odczytanych z
 * czujników BLE, tj. czujników paletowych oraz centrali pomocnicznej. Zawiera
 * identyfikator firmy, identyfikator wariantu, identyfikator węzła oraz
 * specyficzne dane dla każdego typu czujnika.
 */
struct __attribute__((packed)) sensor_data_ble_t {
    uint16_t company_id;
    uint8_t variant_id;
    uint32_t node_id;
    union {
        struct __attribute__((packed)) {
            uint8_t motion_detected;
            int16_t x;
            int16_t y;
            int16_t z;
        } accel;
        struct __attribute__((packed)) {
            int16_t temp;
            uint16_t humid;
            uint8_t is_closed;
        } env;
    } specific;
};

/** @brief Klasa zarządzająca czujnikami BLE.
 *
 * @details Klasa BLESensorManager dziedziczy po klasie
 * BLEAdvertisedDeviceCallbacks i implementuje metodę onResult, która jest
 * wywoływana, gdy zostanie wykryte nowe urządzenie BLE. Klasa przechowuje
 * wskaźniki do obiektów czujników BLEPalletSensor i BLESecondaryCentral oraz
 * oczekiwane adresy MAC tych czujników. Dodatkowo posiada flagę gotPackage,
 * która informuje, czy odebrano pakiet danych od czujników.
 */
class BLESensorManager : public BLEAdvertisedDeviceCallbacks {
   private:
    BLEPalletSensor* palletSensor1;
    BLESecondaryCentral* secondaryCentral;
    std::string expectedMacPallet1;
    std::string expectedMacSecondaryCentral;
    bool gotPackage;

   public:
    /** @brief Konstruktor klasy BLESensorManager.
     *
     * @param[in] palletSensor1 Wskaźnik do obiektu czujnika paletowego.
     * @param[in] secondaryCentral Wskaźnik do obiektu centrali pomocniczej.
     * @param[in] PalletMACId1 Adres MAC czujnika paletowego.
     * @param[in] SecondaryCentralMACId Adres MAC centrali pomocniczej.
     */
    BLESensorManager(BLEPalletSensor* palletSensor1,
                     BLESecondaryCentral* secondaryCentral,
                     std::string PalletMACId1,
                     std::string SecondaryCentralMACId);

    /** @brief Metoda wywoływana, gdy zostanie wykryte nowe urządzenie BLE.
     *
     * @details Metoda onResult jest wywoływana automatycznie przez bibliotekę
     * BLE, gdy zostanie wykryte nowe urządzenie BLE. Sprawdza, czy adres MAC
     * wykrytego urządzenia odpowiada oczekiwanym adresom czujników. Jeśli tak,
     * odczytuje dane z pakietu reklamowego i aktualizuje odpowiednie obiekty
     * czujników. Ustawia również flagę gotPackage na true, aby wskazać, że
     * odebrano pakiet danych.
     *
     * @param[in] advertisedDevice Obiekt reprezentujący wykryte urządzenie BLE.
     */
    void onResult(BLEAdvertisedDevice advertisedDevice) override;

    /** @brief Pobiera i czyści flagę odebranego pakietu.
     *
     * @return Wartość flagi gotPackage.
     */
    bool getAndClearPackageFlag();
};