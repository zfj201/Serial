#include <Arduino.h>
#include "config.h"
#include "module.h"
#include "serial_command.h"
#include "reaction_speed.h"
#include "breathe_led.h"
#include "switch_led_mode.h"
#include "wifi_module.h"
#include "esp_hot_spot.h"
#include "webserver_module.h"
#include "web_button_module.h"
#include "ble_module.h"
#include "chip_temp_module.h"
#include "i2c_scan_module.h"

static const AppModule kModules[] = {
    {MODULE_SERIAL_COMMAND, "serial_command", "串口命令 led_on/off/toggle",
     serialCommandSetup, serialCommandLoop},
    {MODULE_REACTION_SPEED, "reaction_speed", "BOOT 反应速度游戏",
     reactionSpeedSetup, reactionSpeedLoop},
    {MODULE_BREATHE_LED, "breathe_led", "呼吸灯",
     breatheLedSetup, breatheLedLoop},
    {MODULE_SWITCH_LED_MODE, "switch_led_mode", "切换LED模式",
     switchLedModeSetup, switchLedModeLoop},
    {MODULE_WIFI, "wifi", "WIFI",
     wifiSetup, wifiLoop},
    {MODULE_ESP_HOT_SPOT, "esp_hot_spot", "ESP32-AP",
     espHotSpotSetup, espHotSpotLoop},
    {MODULE_WEBSERVER, "webserver", "Web Server",
     webserverSetup, webserverLoop},
    {MODULE_WEB_BUTTON, "web_button", "网页按钮调 API 控 LED",
     webButtonSetup, webButtonLoop},
    {MODULE_BLE, "ble", "BLE 广播 nRF 可见",
     bleSetup, bleLoop},
    {MODULE_CHIP_TEMP, "chip_temp", "芯片温度 2s 采样 Notify",
     chipTempSetup, chipTempLoop},
    {MODULE_I2C_SCAN, "I2C_scan", "I2C 总线地址扫描",
     i2cScanSetup, i2cScanLoop},
};

const AppModule *findModule(int id) {
  for (const AppModule &module : kModules) {
    if (module.id == id) {
      return &module;
    }
  }
  return &kModules[0];
}

const AppModule *moduleByIndex(size_t index) {
  if (index >= sizeof(kModules) / sizeof(kModules[0])) {
    return nullptr;
  }
  return &kModules[index];
}

size_t moduleCount() {
  return sizeof(kModules) / sizeof(kModules[0]);
}

void printModuleMenu() {
  Serial.println();
  Serial.println("=== ESP32-C3 Learning Modules ===");
  for (size_t i = 0; i < moduleCount(); ++i) {
    const AppModule *module = moduleByIndex(i);
    Serial.print("  ");
    Serial.print(module->id);
    Serial.print(") ");
    Serial.print(module->name);
    Serial.print(" - ");
    Serial.println(module->description);
  }
  Serial.print("Send module number within ");
  Serial.print(MODULE_PICK_TIMEOUT_MS / 1000);
  Serial.print("s (default ");
  Serial.print(ACTIVE_MODULE);
  Serial.println("):");
}

int pickModule(int defaultId, unsigned long timeoutMs) {
  printModuleMenu();

  if (timeoutMs == 0) {
    return defaultId;
  }

  String input;
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        input.trim();
        if (input.length() > 0) {
          return findModule(input.toInt())->id;
        }
      } else {
        input += c;
      }
    }
  }

  Serial.print("Using default module ");
  Serial.println(defaultId);
  return defaultId;
}
