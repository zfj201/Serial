#include <Arduino.h>
#include <WiFi.h>
#include "esp_hot_spot.h"
#include "hardware.h"

static const char *AP_SSID = "ESP32-AP";
static const char *AP_PASS = "12345678";
static const int AP_CHANNEL = 1;
static const int AP_MAX_CLIENTS = 4;

static unsigned long lastReportMs = 0;
static const unsigned long reportIntervalMs = 3000;

void espHotSpotSetup() {
    Serial.begin(115200);
    Serial.println("espHotSpotSetup");

    WiFi.mode(WIFI_AP);
    bool ok = WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, false, AP_MAX_CLIENTS);
    if (!ok) {
        Serial.println("softAP 启动失败");
        return;
    }

    Serial.println("热点已开启");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("密码: ");
    Serial.println(AP_PASS);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.softAPmacAddress());
}

void espHotSpotLoop() {
    unsigned long now = millis();
    if (now - lastReportMs < reportIntervalMs) {
        return;
    }
    lastReportMs = now;

    Serial.print("已连接设备数: ");
    Serial.println(WiFi.softAPgetStationNum());
}
