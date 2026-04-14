#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <Sensor.h>

struct __attribute__((packed)) PalletData {
    uint16_t company_id;
    uint16_t temp;
    uint16_t humid;
    uint16_t tilt;
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
            std::string recivedData = advertisedDevice.getManufacturerData();
            if (recivedData.length() == sizeof(PalletData)) {
                PalletData* excpectedData = (PalletData*)recivedData.data();
                switch (excpectedData->company_id) {
                    case 0xA1B1:
                        Serial.println("Dane z nRf o companyid \"0xA1B1\" zostały przyjęte");
                        sensorBLE1->value1 = excpectedData->temp/10.0;
                        sensorBLE1->value2 = excpectedData->humid;
                        sensorBLE1->value3 = excpectedData->tilt;
                        break;
                    case 0xA2B2:
                        Serial.println("Dane z nRf o companyid \"0xA2B2\" zostały przyjęte");
                        sensorBLE2->value1 = excpectedData->temp/10.0;
                        sensorBLE2->value2 = excpectedData->humid;
                        sensorBLE2->value3 = excpectedData->tilt;
                        break;
                    case 0xA3B3:
                        Serial.println("Dane z nRf o companyid \"0xA3B3\" zostały przyjęte");
                        sensorBLE3->value1 = excpectedData->temp/10.0;
                        sensorBLE3->value2 = excpectedData->humid;
                        sensorBLE3->value3 = excpectedData->tilt;
                        break;
                    default:
                        Serial.printf("Znaleziono paczke o companyid: %X\n", excpectedData->company_id);
                        break;
                }
            }
        }
    };
};