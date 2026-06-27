#include "../include/sensors/PresenceSensor.h"

PresenceSensor::PresenceSensor(uint8_t pin)
    : GPIO_pin(pin), isSomeOneHere(false) {}

bool PresenceSensor::initialize() {
    pinMode(GPIO_pin, INPUT);
    return true;
}

void PresenceSensor::readData() {
    isSomeOneHere = (digitalRead(GPIO_pin) == HIGH);
}

bool PresenceSensor::getPresence() const {
    return isSomeOneHere;
}