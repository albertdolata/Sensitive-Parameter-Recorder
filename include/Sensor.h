#pragma once
#include <Arduino.h>

class Sensor {
protected:
String sensorName;

//tymczasowe pola dla wartości z czujników - w przyszłości do zmiany może na jakąś tablice do przemyślenia
float value1;
float value2;
float value3;

public:
Sensor(String name){
    sensorName = name;
}

virtual ~Sensor() {}

virtual bool initialize () = 0;

virtual void readData () = 0;

};