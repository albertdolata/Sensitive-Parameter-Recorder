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
    BLEPalletSensor* palletSensor2;
    BLESecondaryCentral* secondaryCentral;
    uint32_t expectedNodeId1;
    uint32_t expectedNodeId2;

   public:
    BLESensorManager(BLEPalletSensor* palletSensor1,
                     BLEPalletSensor* palletSensor2,
                     BLESecondaryCentral* secondaryCentral, uint32_t NodeId1,
                     uint32_t NodeId2)
        : palletSensor1(palletSensor1),
          palletSensor2(palletSensor2),
          secondaryCentral(secondaryCentral),
          expectedNodeId1(NodeId1),
          expectedNodeId2(NodeId2) {}

    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveManufacturerData()) {
            std::string recivedData = advertisedDevice.getManufacturerData();
            if (recivedData.length() == sizeof(sensor_data_ble_t)) {
                sensor_data_ble_t* expectedData =
                    (sensor_data_ble_t*)recivedData.data();
                if (expectedData->company_id == 0xA1B1) {
                    switch (expectedData->variant_id) {
                        case 0x01:
                            if (expectedData->node_id == expectedNodeId1) {
                                palletSensor1->updateDataFromBLE(
                                    expectedData->specific.accel.x,
                                    expectedData->specific.accel.y,
                                    expectedData->specific.accel.z,
                                    expectedData->specific.accel
                                        .motion_detected);
                            } else if (expectedData->node_id ==
                                       expectedNodeId2) {
                                palletSensor2->updateDataFromBLE(
                                    expectedData->specific.accel.x,
                                    expectedData->specific.accel.y,
                                    expectedData->specific.accel.z,
                                    expectedData->specific.accel
                                        .motion_detected);
                            } else {
                                Serial.printf(
                                    "Nieznaleziono żadnego czujnika "
                                    "paletowego z node_id: %X\n",
                                    expectedData->node_id);
                            }
                            break;
                        case 0x02:
                            secondaryCentral->updateDataFromBLE(
                                expectedData->specific.env.temp,
                                expectedData->specific.env.humid,
                                expectedData->specific.env.is_closed);
                            break;
                        default:
                            Serial.printf(
                                "Nie wykryto czujnika paletowego ani centrali "
                                "pomocnicznej: %X\n",
                                expectedData->company_id);
                            break;
                    }
                }
            }
        }
    };
};