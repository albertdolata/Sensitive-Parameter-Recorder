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
#define NUMBER_OF_SENSORS 5

class MainCentralSensorManager {
   private:
    TempHumSensor mainEnvSensor;
    PresenceSensor presenceSensor;
    AccelSensor mainAccelSensor;

    BLEPalletSensor palletSensor1;
    BLESecondaryCentral secondaryCentral;
    BLESensorManager BLEManager;

    Sensor* sensors[NUMBER_OF_SENSORS];

   public:
    MainCentralSensorManager(uint8_t presence_sensor_pin,
                             uint8_t accel_sensor_i2c_addr,
                             const std::string& mac_pallet1,
                             const std::string& mac_secondary);

    bool initializeAllSensors();

    void readAllSensorsData();

    BLESensorManager* getBLESensorManager();

    void fillSensorData(sensor_data_t* data);

    bool BleGotPackage();
};