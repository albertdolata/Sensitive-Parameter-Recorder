#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>

struct __attribute__((packed)) PalletData {
    uint16_t company_id;
    uint16_t temp;
    uint16_t humid;
    bool red_switch_stat;
};

class BLESensorManager : public BLEAdvertisedDeviceCallbacks {
    private:
    public:
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveManufacturerData()) {
            std::string rawData = advertisedDevice.getManufacturerData();
            if (rawData.length() == sizeof(PalletData)) {
                PalletData* incoming = (PalletData*)rawData.data();
                if(incoming->company_id == 0XABCD){
                    Serial.printf("T: %u, H: %u, S: %d", incoming->temp, incoming->humid, incoming->red_switch_stat);
                }
            }
        }
    };
};