#pragma once

class Sensor {
public:

Sensor(){}

virtual ~Sensor() {}

virtual bool initialize () = 0;

virtual void readData () = 0;

};