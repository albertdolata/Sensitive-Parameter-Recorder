/**
 * @file GPSManager.cpp
 * @brief Plik źródłowy zawierający definicje metod klasy GPSManager.
 */

#include "../include/managers/GPSManager.h"

void GPSManager::parseTime(String rawTime) {
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

void GPSManager::parseResponse(String response) {
    int commaCounter;
    String rawTime;
    String rawLatitdue;
    String rawLongitude;
    if (response.indexOf("+CGNSINF:") != -1) {
        commaCounter = response.indexOf(',');
        int nextComma = response.indexOf(',', commaCounter + 1);
        if (response.substring(commaCounter + 1, nextComma) == "1") {
            fixStatus = true;
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
            GPS_valid = true;
            parseTime(rawTime);
        } else {
            fixStatus = false;
            GPS_valid = false;
        }
    }
}

GPSManager::GPSManager()
    : latitude(0.0), longitude(0.0), isON(false), fixStatus(false) {}

void GPSManager::begin() {
    char rx_buff[128];

    send_at_cmd("AT+CGNSPWR=1\r\n", rx_buff, sizeof(rx_buff), 2000);
    isON = true;
}

void GPSManager::update() {
    if (!isON) return;
    char rx_buff[256];

    if (send_at_cmd("AT+CGNSINF\r\n", rx_buff, sizeof(rx_buff), 2000) > 0) {
        String response(rx_buff);
        parseResponse(response);
    }
}

void GPSManager::pause() {
    if (!isON) return;
    char rx_buff[128];
    send_at_cmd("AT+CGNSPWR=0\r\n", rx_buff, sizeof(rx_buff), 2000);
    isON = false;
    fixStatus = false;
    GPS_valid = false;
}

void GPSManager::resume() {
    if (isON) return;
    fixStatus = false;
    GPS_valid = false;
    timestamp = 0;
    char rx_buff[128];
    send_at_cmd("AT+CGNSPWR=1\r\n", rx_buff, sizeof(rx_buff), 2000);
    isON = true;
}

double GPSManager::getLatitude() {
    return latitude;
}

double GPSManager::getLongitude() {
    return longitude;
}

uint32_t GPSManager::getTimestamp() {
    return timestamp;
}

bool GPSManager::hasFix() {
    return fixStatus;
}

bool GPSManager::isPowered() {
    return isON;
}

bool GPSManager::isGPSValid() {
    return GPS_valid;
}