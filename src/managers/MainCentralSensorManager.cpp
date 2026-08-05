/**
 * @file MainCentralSensorManager.cpp
 * @brief Plik źródłowy zawierający definicje metod klasy MainCentralSensorManager.
 */

#include "../include/managers/MainCentralSensorManager.h"

MainCentralSensorManager::MainCentralSensorManager(
    uint8_t presence_sensor_pin, uint8_t accel_sensor_i2c_addr,
    const std::string& mac_pallet1, const std::string& mac_secondary)
    : presenceSensor(presence_sensor_pin),
      mainAccelSensor(accel_sensor_i2c_addr),
      BLEManager(&palletSensor1, &secondaryCentral, mac_pallet1,
                 mac_secondary) {
    sensors[0] = &mainEnvSensor;
    sensors[1] = &presenceSensor;
    sensors[2] = &mainAccelSensor;
    sensors[3] = &palletSensor1;
    sensors[4] = &secondaryCentral;
}

bool MainCentralSensorManager::initializeAllSensors() {
    for (int i = 0; i < NUMBER_OF_SENSORS; ++i) {
        if (!sensors[i]->initialize()) {
            return false;
        }
    }
    return true;
}
void MainCentralSensorManager::readAllSensorsData() {
    for (int i = 0; i < NUMBER_OF_SENSORS; ++i) {
        sensors[i]->readData();
    }
}

BLESensorManager* MainCentralSensorManager::getBLESensorManager() {
    return &BLEManager;
}

void MainCentralSensorManager::fillSensorData(sensor_data_t* data) {
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

bool MainCentralSensorManager::BleGotPackage() {
    return BLEManager.getAndClearPackageFlag();
}