#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <Sensor.h>

struct __attribute__((packed)) PalletData {
    uint16_t company_id;
    uint16_t temp;
    uint16_t humid;
    bool red_switch_stat;
};

class BLESensorManager : public BLEAdvertisedDeviceCallbacks {
   private:
    Sensor* sensorBLE1;
    Sensor* sensorBLE2;
    Sensor* sensorBLE3;

   public:
    BLESensorManager(Sensor* sensor1, Sensor* sensor2, Sensor* sensor3)
        : sensorBLE1(sensor1), sensorBLE2(sensor2), sensorBLE3(sensor3) {}

    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveManufacturerData()) {
            std::string rawData = advertisedDevice.getManufacturerData();
            if (rawData.length() == sizeof(PalletData)) {
                PalletData* incoming = (PalletData*)rawData.data();
                switch (incoming->company_id) {
                    case 0xA1B1:
                        sensorBLE1->value1 = incoming->temp;
                        sensorBLE1->value2 = incoming->humid;
                        sensorBLE1->value3 = incoming->red_switch_stat;
                        break;
                    case 0xA2B2:
                        sensorBLE2->value1 = incoming->temp;
                        sensorBLE2->value2 = incoming->humid;
                        sensorBLE2->value3 = incoming->red_switch_stat;
                        break;
                    case 0xA3B3:
                        sensorBLE3->value1 = incoming->temp;
                        sensorBLE3->value2 = incoming->humid;
                        sensorBLE3->value3 = incoming->red_switch_stat;
                        break;
                    default:
                        break;
                }
            }
        }
    };
};