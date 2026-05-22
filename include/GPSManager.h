#pragma once
#include <Arduino.h>

#include "sim7000_core.h"

class GPSManager {
   private:
    double latitude;
    double longitude;
    bool isON;

    void parseResponse(String response) {
        int commaCounter;
        String rawTime;
        String rawLatitdue;
        String rawLongitude;
        if (response.indexOf("+CGNSINF:") != -1) {
            commaCounter = response.indexOf(',');
            int nextComma = response.indexOf(',', commaCounter + 1);
            if (response.substring(commaCounter + 1, nextComma) == "1") {
                isON = true;
                commaCounter = nextComma;
                nextComma = response.indexOf(',', commaCounter + 1);
                rawTime = response.substring(commaCounter + 1, nextComma);
                commaCounter = nextComma;
                nextComma = response.indexOf(',', commaCounter + 1);
                rawLatitdue = response.substring(commaCounter + 1, nextComma);
                commaCounter = nextComma;
                nextComma = response.indexOf(',', commaCounter + 1);
                rawLongitude = response.substring(commaCounter + 1, nextComma);

                latitude = rawLatitdue.toDouble();
                longitude = rawLongitude.toDouble();
            } else {
                isON = false;
            }
        }
    }

   public:
    GPSManager() : latitude(0.0), longitude(0.0), isON(false) {}

    void begin() {
        char rx_buff[128];

        send_at_cmd("AT+CGNSPWR=1\r\n", rx_buff, sizeof(rx_buff), 2000);

        Serial.println("[DEBUG] GPS pomyslnie zainicjowany.");
    }

    void update() {
        char rx_buff[256];

        if(send_at_cmd("AT+CGNSINF\r\n", rx_buff, sizeof(rx_buff), 2000) > 0) {
            String response(rx_buff);
            parseResponse(response);
        }
    }

    double getLatitude() {
        return latitude;
    }

    double getLongitude() {
        return longitude;
    }

    bool hasFix() {
        return isON;
    }
};