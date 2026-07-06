// 芯片内置温度：每 2 秒读取 ESP32-C3 内部传感器，串口打印并通过 BLE Notify 推送
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "chip_temp_module.h"

static const char *SERVICE_UUID = "a1b2c3d4-e5f6-4789-a012-3456789abcde";
static const char *CHAR_UUID = "b2c3d4e5-f6a7-4890-b123-456789abcdef";
static const char *DEVICE_NAME = "ESP32-C3-TEMP";

static constexpr unsigned long kSampleIntervalMs = 2000;

static BLECharacteristic *tempChar = nullptr;
static bool deviceConnected = false;

class TempServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *server) override {
        deviceConnected = true;
        Serial.println("BLE 已连接");
    }

    void onDisconnect(BLEServer *server) override {
        deviceConnected = false;
        Serial.println("BLE 已断开，重新广播");
        BLEDevice::startAdvertising();
    }
};

void chipTempSetup() {
    Serial.begin(115200);
    Serial.println("chipTempSetup");

    BLEDevice::init(DEVICE_NAME);

    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new TempServerCallbacks());

    BLEService *service = server->createService(SERVICE_UUID);

    tempChar = service->createCharacteristic(
        CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    tempChar->addDescriptor(new BLE2902());
    tempChar->setValue("0");

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE 广播中，nRF Connect 扫描找: " + String(DEVICE_NAME));
    Serial.println("每 2 秒读取芯片温度，串口打印并 Notify（需先订阅特征值）");
}

void chipTempLoop() {
    static unsigned long lastSampleMs = 0;
    if (millis() - lastSampleMs < kSampleIntervalMs) {
        return;
    }
    lastSampleMs = millis();

    float tempC = temperatureRead();
    Serial.print("芯片温度: ");
    Serial.print(tempC, 1);
    Serial.println(" C");

    if (!deviceConnected || !tempChar) {
        return;
    }

    // 文本格式 Notify，nRF Connect 直接显示如 "43.5 C"
    char tempText[16];
    snprintf(tempText, sizeof(tempText), "%.1f C", tempC);
    tempChar->setValue(tempText);
    tempChar->notify();
}
