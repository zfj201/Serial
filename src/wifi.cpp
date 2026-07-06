#include <Arduino.h>
#include <WiFi.h>
#include "wifi_module.h"
#include "hardware.h"

void wifiSetup() {
    Serial.begin(115200);
    Serial.println("wifiSetup");
    int count = WiFi.scanNetworks();
    WiFi.begin("2801", "18367168360");
    Serial.print("连接中");
    unsigned long startMs = millis();
    unsigned long lastDotMs = startMs;
    const unsigned long dotIntervalMs = 500;
    const unsigned long timeoutMs = 15000;

    while (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastDotMs >= dotIntervalMs) {
            lastDotMs = now;
            Serial.print(".");
        }
        if (now - startMs >= timeoutMs) {
            Serial.println(" 超时");
            return;
        }
    }
  
    Serial.println();
    Serial.println("连接成功！");
    Serial.print("WiFi.status: ");
    Serial.println(WiFi.status());
    Serial.println("WiFi.localIP: " + WiFi.localIP().toString());
    Serial.println("scanNetworks: " + String(count));
    for (int i = 0; i < count; i++) {
        Serial.print("WiFi.SSID: " + WiFi.SSID(i));
        Serial.println("WiFi.RSSI: " + String(WiFi.RSSI(i)));
        Serial.println("WiFi.encryptionType: " + String(WiFi.encryptionType(i)));
        Serial.println("WiFi.channel: " + String(WiFi.channel(i)));
        Serial.println("WiFi.BSSID: " + WiFi.BSSIDstr(i));
    }
    Serial.println("WiFi.macAddress: " + WiFi.macAddress());
    Serial.println("WiFi.isConnected: " + String(WiFi.isConnected()));
    Serial.println("WiFi.getMode: " + String(WiFi.getMode()));
    Serial.println("WiFi.getHostname: " + String(WiFi.getHostname()));
    Serial.println("WiFi.gatewayIP: " + WiFi.gatewayIP().toString());
    Serial.println("WiFi.subnetMask: " + WiFi.subnetMask().toString());
}

void wifiLoop() {
    // Serial.println("wifiLoop");
}