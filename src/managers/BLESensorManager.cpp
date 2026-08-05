/**
 * @file BLESensorManager.cpp
 * @brief Plik implementacyjny zawierający definicje metod klasy BLESensorManager.
 */

#include "../include/managers/BLESensorManager.h"

BLESensorManager::BLESensorManager(BLEPalletSensor* palletSensor1,
                                   BLESecondaryCentral* secondaryCentral,
                                   std::string PalletMACId1,
                                   std::string SecondaryCentralMACId)
    : palletSensor1(palletSensor1),
      secondaryCentral(secondaryCentral),
      expectedMacPallet1(PalletMACId1),
      expectedMacSecondaryCentral(SecondaryCentralMACId),
      gotPackage(false) {}

void BLESensorManager::onResult(BLEAdvertisedDevice advertisedDevice) {
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
                gotPackage = true;
            } else if (deviceAddress == expectedMacSecondaryCentral) {
                secondaryCentral->updateDataFromBLE(
                    incomingData->specific.env.temp,
                    incomingData->specific.env.humid,
                    incomingData->specific.env.is_closed);
                gotPackage = true;
            }
        }
    }
}

bool BLESensorManager::getAndClearPackageFlag() {
    if (gotPackage) {
        gotPackage = false;
        return true;
    } else {
        return false;
    }
}