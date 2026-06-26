#pragma once

#include "AccelSensor.h"
#include "BLEPalletSensor.h"
#include "BLESecondaryCentral.h"
#include "BLESensorManager.h"
#include "PresenceSensor.h"
#include "TempHumSensor.h"
#include "data_send_service.h"

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
                             std::string mac_pallet1, 
                             std::string mac_secondary)
        : presenceSensor(presence_sensor_pin),
          mainAccelSensor(accel_sensor_i2c_addr),
          BLEManager(&palletSensor1, &secondaryCentral,
                     mac_pallet1, mac_secondary) {
        sensors[0] = &mainEnvSensor;
        sensors[1] = &presenceSensor;
        sensors[2] = &mainAccelSensor;
        sensors[3] = &palletSensor1;
        sensors[4] = &secondaryCentral;
    }

    bool initializeAllSensors() {
        for (int i = 0; i < NUMBER_OF_SENSORS; ++i) {
            if (!sensors[i]->initialize()) {
                return false;
            }
        }
        return true;
    }
    void readAllSensorsData() {
        for (int i = 0; i < NUMBER_OF_SENSORS; ++i) {
            sensors[i]->readData();
        }
    }

    BLESensorManager* getBLESensorManager() {
        return &BLEManager;
    }

    void fillSensorData(sensor_data_t* data) {
        data->temperature_main_central = mainEnvSensor.getTemperature();
        data->humidity_main_central = mainEnvSensor.getHumidity();
        data->accelx_main_central = mainAccelSensor.getAxisX();
        data->accely_main_central = mainAccelSensor.getAxisY();
        data->accelz_main_central = mainAccelSensor.getAxisZ();
        data->presence_main_central = presenceSensor.getPresence();

        data->temperature_secondary_central = secondaryCentral.getTemp();
        data->humidity_secondary_central = secondaryCentral.getHumidity();
        data->is_closed_secondary_central = secondaryCentral.getIsClosed();

        data->accelx_palette1 = palletSensor1.getAxisX();
        data->accely_palette1 = palletSensor1.getAxisY();
        data->accelz_palette1 = palletSensor1.getAxisZ();
        data->motion_detected_p1 = (palletSensor1.isMotionDetected() == true);
    }
};