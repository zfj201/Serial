// 基础 BLE：板子当「蓝牙外设」，手机 nRF Connect 扫描、连接、写特征值控 LED
//
// BLE 层次（由外到内）：
//   Device（设备名 ESP32-C3-BLE）
//     → Service（一组功能，用 SERVICE_UUID 标识）
//       → Characteristic（具体数据/命令，用 CHAR_UUID 标识）
#include <Arduino.h>
#include <BLEDevice.h>   // 蓝牙入口：init / 广播
#include <BLEServer.h>   // GATT Server，管理连接
#include <BLEUtils.h>
#include <BLE2902.h>     // Client Characteristic Configuration，Notify 订阅用
#include "ble_module.h"
#include "hardware.h"

// Service UUID：nRF Connect 连接后看到的「服务」
static const char *SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c5c3709346";
// Characteristic UUID：服务里的「特征值」，手机对它 Read/Write
// 必须是标准 36 字符格式 8-4-4-4-12，多/少一位会导致 BLEUUID 报错并 panic 重启
static const char *CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea0736814799";
// 广播名：nRF Connect 扫描列表里显示的名字
static const char *DEVICE_NAME = "ESP32-C3-BLE";

static BLECharacteristic *ledChar = nullptr;  // 保存指针，进阶 Notify 时要用
static bool deviceConnected = false;          // 当前是否有手机连着

// 连接状态回调：手机连上/断开时触发（不是写在 loop 里的）
class BleServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *server) override {
        deviceConnected = true;
        Serial.println("BLE 已连接");
    }

    void onDisconnect(BLEServer *server) override {
        deviceConnected = false;
        Serial.println("BLE 已断开，重新广播");
        // 断开后广播会停，必须手动再开，否则 nRF 扫描列表里看不到设备
        BLEDevice::startAdvertising();
    }
};

// 特征值写入回调：手机在 nRF Connect 里 Write 时触发
class LedCharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) override {
        std::string value = characteristic->getValue();
        if (value.empty()) {
            return;
        }
        // nRF Connect 选 BYTE，写 01=开灯，00=关灯
        uint8_t cmd = value[0];
        bool on = (cmd != 0);
        digitalWrite(PIN_LED1, on ? HIGH : LOW);

        // Notify：把当前 LED 状态推回手机（需 nRF Connect 先开启 Notify 订阅）
        uint8_t status = on ? 1 : 0;
        characteristic->setValue(&status, 1);
        characteristic->notify();

        Serial.print("BLE 写入: ");
        Serial.println(on ? "ON" : "OFF");
    }
};

void bleSetup() {
    Serial.begin(115200);
    Serial.println("bleSetup");

    pinMode(PIN_LED1, OUTPUT);
    digitalWrite(PIN_LED1, LOW);

    // ① 初始化蓝牙协议栈，参数为广播设备名
    BLEDevice::init(DEVICE_NAME);

    // ② 创建 GATT Server（板子是 Server，手机是 Client）
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new BleServerCallbacks());

    // ③ 创建一个 Service，并挂上 Characteristic
    BLEService *service = server->createService(SERVICE_UUID);

    ledChar = service->createCharacteristic(
        CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |   // 手机可读
            BLECharacteristic::PROPERTY_WRITE |  // 手机可写（控 LED 靠这个）
            BLECharacteristic::PROPERTY_NOTIFY); // 板子可主动推数据给手机
    ledChar->setCallbacks(new LedCharCallbacks());
    ledChar->addDescriptor(new BLE2902());  // Notify 必备描述符，否则无法订阅通知
    ledChar->setValue("0");                 // 初始值，Read 时能看到

    service->start();  // 服务就绪，但还要 startAdvertising 才能被扫到

    // ④ 配置并开启广播（Advertising = 不断发「我在这里」的无线电信号）
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);  // 广播包里带上 Service UUID，方便过滤
    advertising->setScanResponse(true);         // 扫描响应，可带更多设备信息
    advertising->setMinPreferred(0x06);         // 连接间隔偏好，官方示例值
    advertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE 广播中，nRF Connect 扫描找: " + String(DEVICE_NAME));
    Serial.println("Service UUID: " + String(SERVICE_UUID));
    // nRF Connect：Connect → 展开 Service → 点特征值 beb5483e...
    // → 先打开 Notify 订阅（三个向下箭头）→ Write BYTE 01/00 → 会收到 Notify 回传
}

void bleLoop() {
    // 定时 Notify：每秒推送一次 LED 状态（连接且已订阅时手机才收得到）
    static unsigned long lastNotifyMs = 0;
    if (!deviceConnected || !ledChar) {
        return;
    }
    if (millis() - lastNotifyMs < 1000) {
        return;
    }
    lastNotifyMs = millis();

    uint8_t status = digitalRead(PIN_LED1) ? 1 : 0;
    ledChar->setValue(&status, 1);
    ledChar->notify();
}
