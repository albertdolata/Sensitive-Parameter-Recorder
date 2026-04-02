#pragma once

#include <ThingSpeak.h>
#include <WiFi.h>

class CloudManager {
   private:
    WiFiClient client;

   public:
    CloudManager() {}

    void connectWiFi(const char* ssid, const char* password) {
        Serial.println("Łączenie z siecią Wi-Fi...");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.println(".");
        }
        Serial.printf("Połączono z siecią  o ip: %s", WiFi.localIP().toString().c_str());
    }

    void initCloud() {
        ThingSpeak.begin(client);
        Serial.print("Serwer gotowy do wysyłania danych.");
    }

};