// BOOT 按钮循环切换 LED 模式：关 / 常亮 / 闪烁 / 呼吸（PWM）
#include "switch_led_mode.h"
#include "hardware.h"
#include <Arduino.h>

// 当前模式 0~3；prevMode 用于检测模式是否刚切换，以便做一次性初始化
static int ledMode = 0;
static int prevMode = -1;

// 按键消抖：信号稳定 delayTime 毫秒后才认可一次状态变化
const int delayTime = 10;
unsigned long lastTime = 0;
int lastReading = HIGH;
int lastBtnState = HIGH;

// 呼吸灯用 ESP32 LEDC（PWM）：8 位分辨率 → 亮度 0~255
const int frequency = 5000;
const int resolution = 8;
const int channel = 0;
const int blinkIntervalMs = 500;   // 闪烁半周期（亮/灭各 500ms）
const int breatheIntervalMs = 10;  // 呼吸亮度更新间隔

static int step = 5;               // 每次亮度增减步长，到顶/到底后取反实现往返
static int brightness = 0;
static unsigned long lastBlinkMs = 0;
static bool blinkOn = false;
static unsigned long lastBreatheMs = 0;

static void prepareLedMode();
static void handleButton();
static void handleLedMode();
void switchLedModeSetup() {
    Serial.begin(115200);
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    pinMode(PIN_BOOT, INPUT_PULLUP);  // 内部上拉，按下读 LOW
    Serial.println("switchLedModeSetup");
    // PWM 仅在进入模式 3（呼吸）时 attach，见 prepareLedMode()
}

void switchLedModeLoop() {
    // Serial.println("switchLedModeLoop");
    handleButton();
    handleLedMode();
}

// 检测 BOOT 按下沿（HIGH→LOW），每按一次切换到下一模式
static void handleButton() {
    unsigned long currentTime = millis();
    int reading = digitalRead(PIN_BOOT);

    // 电平变化时重置计时，等待稳定 delayTime 后再采信
    if (reading != lastReading) {
        lastTime = currentTime;
    }
    lastReading = reading;

    if (currentTime - lastTime > delayTime) {
        // 仅上升沿后的第一次稳定 LOW 计为一次按下，避免长按连跳
        if (reading == LOW && lastBtnState == HIGH) {
            ledMode++;
            if (ledMode > 3) {
                ledMode = 0;
            }
            Serial.println("ledMode: " + String(ledMode));
        }
        lastBtnState = reading;
    }
}

// 模式刚切换时执行一次性准备（PWM  attach/detach、计时器复位等）
static void prepareLedMode() {
    if (ledMode == prevMode) {
        return;
    }

    // 离开呼吸模式：解除 PWM，恢复普通 GPIO，否则 digitalWrite 无效
    if (prevMode == 3) {
        ledcDetachPin(PIN_LED1);
        pinMode(PIN_LED1, OUTPUT);
    }

    if (ledMode == 2) {
        // 进入闪烁：从灭态开始，重置计时
        lastBlinkMs = millis();
        blinkOn = false;
        digitalWrite(PIN_LED1, LOW);
        digitalWrite(PIN_LED2, LOW);
    } else if (ledMode == 3) {
        // 进入呼吸：占用 LED1 做 PWM，从 0 亮度渐亮
        ledcSetup(channel, frequency, resolution);
        ledcAttachPin(PIN_LED1, channel);
        brightness = 0;
        step = 5;
        lastBreatheMs = millis();
        ledcWrite(channel, brightness);
    }

    prevMode = ledMode;
}

// 每帧根据当前模式更新 LED（非阻塞，用 millis 控制节奏）
static void handleLedMode() {
    prepareLedMode();

    switch (ledMode) {
        case 0:  // 全灭
            digitalWrite(PIN_LED1, LOW);
            digitalWrite(PIN_LED2, LOW);
            break;
        case 1:  // 全亮（两路 GPIO 高电平）
            digitalWrite(PIN_LED1, HIGH);
            digitalWrite(PIN_LED2, HIGH);
            break;
        case 2:  // 闪烁：每隔 blinkIntervalMs 翻转一次亮灭
            if (millis() - lastBlinkMs >= (unsigned long)blinkIntervalMs) {
                lastBlinkMs = millis();
                blinkOn = !blinkOn;
                digitalWrite(PIN_LED1, blinkOn ? HIGH : LOW);
                digitalWrite(PIN_LED2, blinkOn ? HIGH : LOW);
            }
            break;
        case 3:  // 呼吸：LED1 PWM 在 0↔255 间往返；LED2 未接 PWM
            if (millis() - lastBreatheMs >= (unsigned long)breatheIntervalMs) {
                lastBreatheMs = millis();
                brightness += step;
                // 触顶或触底时钳位并反转 step，形成三角波亮度
                if (brightness >= 255) {
                    brightness = 255;
                    step = -step;
                } else if (brightness <= 0) {
                    brightness = 0;
                    step = -step;
                }
                ledcWrite(channel, brightness);
            }
            break;
    }
}
