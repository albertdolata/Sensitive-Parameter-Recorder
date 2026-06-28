#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>

#include "../sensors/BLEPalletSensor.h"
#include "../sensors/BLESecondaryCentral.h"
#include "../sensors/Sensor.h"

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

class BLESensorManager : public BLEAdvertisedDeviceCallbacks {
   private:
    BLEPalletSensor* palletSensor1;
    BLESecondaryCentral* secondaryCentral;
    std::string expectedMacPallet1;
    std::string expectedMacSecondaryCentral;
    bool gotPackage;

   public:
    BLESensorManager(BLEPalletSensor* palletSensor1,
                     BLESecondaryCentral* secondaryCentral,
                     std::string PalletMACId1,
                     std::string SecondaryCentralMACId);

    void onResult(BLEAdvertisedDevice advertisedDevice) override;

    bool getAndClearPackageFlag();
};