#pragma once
#include <Arduino.h>

class Sensor {
protected:
String sensorName;

public:
//tymczasowe pola dla wartości z czujników - w przyszłości do zmiany może na jakąś tablice do przemyślenia
float value1 = 0;
float value2 = 0;
float value3 = 0;
Sensor(String name){
    sensorName = name;
}

virtual ~Sensor() {}

virtual bool initialize () = 0;

virtual void readData () = 0;

};