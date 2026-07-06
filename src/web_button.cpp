// 网页按钮 + 小接口（API）控制板子
// 与 webserver.cpp 的区别：
//   webserver  访问 /on、/off 等路径，浏览器整页跳转
//   本模块     打开一个页面，点按钮时用 fetch() 后台请求 /api/...，页面不刷新
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "web_button_module.h"
#include "hardware.h"

static const char *WIFI_SSID = "2801";
static const char *WIFI_PASS = "18367168360";
static const unsigned long CONNECT_TIMEOUT_MS = 15000;

static WebServer server(80);
static bool ledOn = false;

static bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("连接 WiFi");
    unsigned long startMs = millis();
    unsigned long lastDotMs = startMs;

    while (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastDotMs >= 500) {
            lastDotMs = now;
            Serial.print(".");
        }
        if (now - startMs >= CONNECT_TIMEOUT_MS) {
            Serial.println(" 超时");
            return false;
        }
    }

    Serial.println();
    Serial.print("已连接，IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

static void setLed(bool on) {
    ledOn = on;
    digitalWrite(PIN_LED1, on ? HIGH : LOW);
}

// 带按钮的控制页：点击后 JavaScript 去调 /api/led/...
static void handlePage() {
    const char *html = R"raw(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 按钮控制</title>
  <style>
    body { font-family: sans-serif; text-align: center; margin-top: 40px; }
    button { font-size: 18px; padding: 12px 24px; margin: 8px; }
    #msg { margin-top: 20px; color: #333; }
  </style>
</head>
<body>
  <h1>ESP32-C3 LED 控制</h1>
  <p>点按钮 → 浏览器后台请求小接口 → 板子改行为</p>
  <button onclick="callApi('/api/led/on')">开灯</button>
  <button onclick="callApi('/api/led/off')">关灯</button>
  <button onclick="callApi('/api/led/toggle')">切换</button>
  <button onclick="callApi('/api/led/status')">查询状态</button>
  <p id="msg">等待操作...</p>
  <script>
    // fetch 只拿数据，不跳转页面（这就是"小接口"的用法）
    async function callApi(path) {
      try {
        const res = await fetch(path);
        const text = await res.text();
        document.getElementById('msg').innerText = text;
      } catch (e) {
        document.getElementById('msg').innerText = '请求失败';
      }
    }
  </script>
</body>
</html>
)raw";
    server.send(200, "text/html", html);
}

static void handleApiLedOn() {
    setLed(true);
    server.send(200, "text/plain", "LED 已打开");
}

static void handleApiLedOff() {
    setLed(false);
    server.send(200, "text/plain", "LED 已关闭");
}

static void handleApiLedToggle() {
    setLed(!ledOn);
    server.send(200, "text/plain", ledOn ? "LED 已打开" : "LED 已关闭");
}

static void handleApiLedStatus() {
    server.send(200, "text/plain", ledOn ? "当前: 亮" : "当前: 灭");
}

static void handleNotFound() {
    server.send(404, "text/plain", "接口不存在");
}

void webButtonSetup() {
    Serial.begin(115200);
    Serial.println("webButtonSetup");

    if (!connectWiFi()) {
        return;
    }

    pinMode(PIN_LED1, OUTPUT);
    setLed(false);

    // 页面路由：只负责展示按钮
    server.on("/", handlePage);

    // API 路由：只负责改板子行为，返回短文本给网页显示
    server.on("/api/led/on", handleApiLedOn);
    server.on("/api/led/off", handleApiLedOff);
    server.on("/api/led/toggle", handleApiLedToggle);
    server.on("/api/led/status", handleApiLedStatus);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("打开浏览器访问上面的 IP，用页面按钮控制");
}

void webButtonLoop() {
    server.handleClient();
}
