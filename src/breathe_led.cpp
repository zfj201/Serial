#include <Arduino.h>
#include "breathe_led.h"
#include "hardware.h"
const int frequency = 5000;
const int resolution = 8;
const int channel = 0;
static int step = 5;
static int brightness = 0;
void breatheLedSetup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_BOOT, INPUT_PULLUP);
//   PWM
    ledcSetup(channel, frequency, resolution);
    ledcAttachPin(PIN_LED1, channel);
    Serial.begin(115200);
    Serial.println("breatheLedSetup");
}

void breatheLedLoop() {
    // 如果BOOT按钮被按下，则呼吸灯开始呼吸
  if(digitalRead(PIN_BOOT) == LOW) {
    Serial.println(" button down");
    brightness += step;
    if(brightness > 255 || brightness <= 0){
        step = -step;
    }
    ledcWrite(channel, brightness);
    delay(10);
  }
//   呼吸灯写法1
//   for (int i = 0; i < 256; i++) {
//     ledcWrite(channel, i);
//     delay(10);
//   }
//   for (int i = 255; i >= 0; i--) {
//     ledcWrite(channel, i);
//     delay(10);
//   }

// 呼吸灯写法2
    // Serial.println(brightness);
    // brightness += step;
    // if(brightness > 255 || brightness <= 0) {
    //     step = -step;
    // }
    // ledcWrite(channel, brightness);
    // delay(10);
}