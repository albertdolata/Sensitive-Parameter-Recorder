#include "../include/services/ble_service.h"

#include <BLEDevice.h>
#include <BLEScan.h>

static BLEScan* pBLEScan = nullptr;

void BLEInit(BLESensorManager* bleManager) {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();

    pBLEScan->setAdvertisedDeviceCallbacks(bleManager);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void bleScanTask(void* parameter) {
    while (true) {
        BLEScanResults foundDevices = pBLEScan->start(6, false);
        pBLEScan->clearResults();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}