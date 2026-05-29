#pragma once
#include <Arduino.h>

#include "sim7000_core.h"

class GPSManager {
   private:
    double latitude;
    double longitude;
    uint32_t timestamp;
    bool isON;

    void parseTime(String rawTime) {
        if (rawTime.length() >= 14) {
            struct tm timeinfo = {0};
            timeinfo.tm_year = rawTime.substring(0, 4).toInt() - 1900;
            timeinfo.tm_mon = rawTime.substring(4, 6).toInt() - 1;
            timeinfo.tm_mday = rawTime.substring(6, 8).toInt();
            timeinfo.tm_hour = rawTime.substring(8, 10).toInt();
            timeinfo.tm_min = rawTime.substring(10, 12).toInt();
            timeinfo.tm_sec = rawTime.substring(12, 14).toInt();
            timestamp = mktime(&timeinfo); 
        } else {
            timestamp = 0;
        }
    }

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
                parseTime(rawTime);
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

    uint32_t getTimestamp() {
        return timestamp;
    }

    bool hasFix() {
        return isON;
    }
};