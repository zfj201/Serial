#include <Arduino.h>
#include <Wire.h>

#include "i2c_scan_module.h"

// ESP32-C3 开发板常用 I2C 引脚（按你板子接线可改）
static const int SDA_PIN = 4;
static const int SCL_PIN = 5;

static constexpr unsigned long kScanIntervalMs = 2000;

static void scanI2C()
{
  uint8_t found = 0;

  Serial.println();
  Serial.println("Scanning I2C bus...");

  for (uint8_t addr = 0x08; addr < 0x78; ++addr)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("I2C device found at address 0x%02X\n", addr);
      ++found;
    }
  }

  Serial.println();
  Serial.print("Found ");
  Serial.print(found);
  Serial.println(" devices.");
}

void i2cScanSetup()
{
  Wire.begin(SDA_PIN, SCL_PIN, 400000);
  Wire.setTimeOut(50);

  Serial.println("i2cScanSetup");
  Serial.printf("I2C pins: SDA=%d SCL=%d\n", SDA_PIN, SCL_PIN);
}

void i2cScanLoop()
{
  static unsigned long lastScanMs = 0;
  if (millis() - lastScanMs < kScanIntervalMs)
  {
    return;
  }
  lastScanMs = millis();

  scanI2C();
}
