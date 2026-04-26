#pragma once
#include <Arduino.h>

class GPSManager {
   private:
    HardwareSerial& modemSerial;
    int rxPin;
    int txPin;
    uint32_t baudRate;
    float latitude;
    float longtitude;
    bool isON;
    String rxBuffer;
    void parseResponse(String response) {
        int commaCounter;
        String rawTime;
        String rawLatitdue;
        String rawLongtitude;
        if (response.startsWith("+CGNSINF:")) {
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
                rawLongtitude = response.substring(commaCounter + 1, nextComma);

                latitude = rawLatitdue.toFloat();
                longtitude = rawLongtitude.toFloat();
            } else {
                isON = false;
            }
        }
    }

   public:
    GPSManager(HardwareSerial& serial, int rxPin, int txPin, uint32_t baud)
        : modemSerial(serial),
          rxPin(rxPin),
          txPin(txPin),
          baudRate(baud),
          latitude(0.0),
          longtitude(0.0),
          isON(false) {}

    void begin() {
        pinMode(27, OUTPUT);
        digitalWrite(27, HIGH);
        delay(2000);
        digitalWrite(27, LOW);
        delay(3000);

        modemSerial.begin(baudRate);
        Serial.println("\n[DEBUG] Czekam na start modemu");

        bool isResponding = false;
        while (!isResponding) {
            modemSerial.println("AT");
            delay(500);

            while (modemSerial.available()) {
                String odpowiedz = modemSerial.readString();
                if (odpowiedz.indexOf("OK") != -1) {
                    isResponding = true;
                    Serial.println(
                        "[DEBUG] Modem sie włączył");
                }
            }
        }

        modemSerial.println("AT+CGNSPWR=1");
        delay(1000);

        while (modemSerial.available()) modemSerial.read();
        Serial.println("[DEBUG] GPS pomyslnie zainicjowany.");
    }

    void update() {
        while (modemSerial.available()) {
            char c = modemSerial.read();
            Serial.print(c);
            rxBuffer += c;
            if (c == '\n') {
                rxBuffer.trim();
                if (rxBuffer.length() > 0) {
                    parseResponse(rxBuffer);
                }
                rxBuffer = "";
            }
        }
    }

    void requestPosition() {
        modemSerial.println("AT+CGNSINF");
    }

    float getLatitude() {
        return latitude;
    }

    float getLongtitude() {
        return longtitude;
    }

    bool hasFix() {
        return isON;
    }
};