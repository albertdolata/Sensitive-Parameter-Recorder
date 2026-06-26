#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>

#include "BLEPalletSensor.h"
#include "BLESecondaryCentral.h"
#include "Sensor.h"

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

   public:
    BLESensorManager(BLEPalletSensor* palletSensor1,
                     BLESecondaryCentral* secondaryCentral,
                     std::string PalletMACId1,
                     std::string SecondaryCentralMACId)
        : palletSensor1(palletSensor1),
          secondaryCentral(secondaryCentral),
          expectedMacPallet1(PalletMACId1),
          expectedMacSecondaryCentral(SecondaryCentralMACId) {}

    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        std::string deviceAddress = advertisedDevice.getAddress().toString();
        if (advertisedDevice.haveManufacturerData()) {
            std::string recivedData = advertisedDevice.getManufacturerData();
            if (recivedData.length() == sizeof(sensor_data_ble_t)) {
                sensor_data_ble_t* incomingData =
                    (sensor_data_ble_t*)recivedData.data();
                if (deviceAddress == expectedMacPallet1) {
                    palletSensor1->updateDataFromBLE(
                        incomingData->specific.accel.x,
                        incomingData->specific.accel.y,
                        incomingData->specific.accel.z,
                        incomingData->specific.accel.motion_detected);
                } else if (deviceAddress == expectedMacSecondaryCentral) {
                    secondaryCentral->updateDataFromBLE(
                        incomingData->specific.env.temp,
                        incomingData->specific.env.humid,
                        incomingData->specific.env.is_closed);
                }
            }
        }
    };
};