#pragma once
#include <Arduino.h>

class Sensor {
protected:
String sensorName;

public:
Sensor(String name){
    sensorName = name;
}

virtual ~Sensor() {}

virtual bool initialize () = 0;

virtual void readData () = 0;

};