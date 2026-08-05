/**
 * @file BLESecondaryCentral.cpp
 * @brief Plik źródłowy zawierający definicje metod klasy BLESecondaryCentral.
 */

#include "../include/sensors/BLESecondaryCentral.h"

BLESecondaryCentral::BLESecondaryCentral()
    : temp(0.0), hum(0.0), isClosed(false) {}

bool BLESecondaryCentral::initialize() {
    return true;
}

void BLESecondaryCentral::readData() {}

float BLESecondaryCentral::getTemp() const {
    return temp;
}

float BLESecondaryCentral::getHumidity() const {
    return hum;
}

bool BLESecondaryCentral::getIsClosed() const {
    return isClosed;
}

void BLESecondaryCentral::updateDataFromBLE(int16_t temp, uint16_t hum,
                                            uint8_t isClosed) {
    this->temp = temp / 100.0f;
    this->hum = hum / 100.0f;
    this->isClosed = (isClosed == 1);
}