// 本模块：先连家里路由器 WiFi，再在本机 80 端口提供网页访问
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>          // ESP32 内置 HTTP 库，注意用尖括号
#include "webserver_module.h"   // 本地头文件故意不用 webserver.h，避免 macOS 上遮蔽 <WebServer.h>
#include "hardware.h"

static const char *WIFI_SSID = "2801";
static const char *WIFI_PASS = "18367168360";
static const unsigned long CONNECT_TIMEOUT_MS = 15000;

// WebServer(80)：监听 80 端口，浏览器访问 http://192.168.x.x 默认就是 80 端口
static WebServer server(80);

// 连接路由器。WiFi.begin() 是异步的，不会立刻连上，所以要循环等待
static bool connectWiFi() {
    // WIFI_STA = Station 模式，ESP32 作为客户端去连路由器（不是开热点）
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("连接 WiFi");
    unsigned long startMs = millis();      // 记录开始时间，用于总超时
    unsigned long lastDotMs = startMs;   // 记录上次打印 "." 的时间

    // WL_CONNECTED = 3，只有变成 3 才表示拿到 IP、可以上网
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();

        // 每 500ms 打印一个点，表示还在连，不用 delay 阻塞
        if (now - lastDotMs >= 500) {
            lastDotMs = now;
            Serial.print(".");
        }

        // 超过 15 秒仍未连上（密码错、信号差等），放弃
        if (now - startMs >= CONNECT_TIMEOUT_MS) {
            Serial.println(" 超时");
            return false;
        }
    }

    Serial.println();
    Serial.print("已连接，IP: ");
    Serial.println(WiFi.localIP());  // 路由器分配的局域网 IP，浏览器要访问这个地址
    return true;
}

// 路由回调：有人访问 "/" 时，WebServer 库会自动调用这个函数
// 注意：这不是 loop 里手动调的，而是 handleClient() 收到请求后触发的
static void handleRoot() {
    String page = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                  "<title>ESP32-C3</title></head><body>"
                  "<h1>Hello from ESP32-C3</h1>"
                  "<p>IP: " + WiFi.localIP().toString() + "</p>"
                  "</body></html>";

    // send(状态码, 内容类型, 正文)
    // 200 = OK；text/html 告诉浏览器按网页解析
    server.send(200, "text/html", page);
}

// 访问了没注册的路径（如 /abc）时触发
static void handleNotFound() {
    server.send(404, "text/plain", "Not Found");
}

static void handleOn() {
    digitalWrite(PIN_LED1, HIGH);
    // 必须 send 回复，否则浏览器会一直转圈等待
    server.send(200, "text/plain", "LED ON");
}

static void handleOff() {
    digitalWrite(PIN_LED1, LOW);
    server.send(200, "text/plain", "LED OFF");
}

static void handleStatus() {
    Serial.println("handleStatus");
    server.send(200, "text/html", "<h1>Status</h1>");
}

void webserverSetup() {
    Serial.begin(115200);
    Serial.println("webserverSetup");

    if (!connectWiFi()) {
        return;  // WiFi 失败就不启动 Web 服务，避免 server 起来但无法访问
    }

    pinMode(PIN_LED1, OUTPUT);
    digitalWrite(PIN_LED1, LOW);

    // 注册路由：路径 → 处理函数（类似"网址目录表"）
    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
    server.on("/status", handleStatus);
    server.onNotFound(handleNotFound);

    // begin() 只启动监听，不会阻塞；真正处理请求在 loop 的 handleClient()
    server.begin();
    Serial.println("HTTP 服务已启动，浏览器打开上面的 IP");
}

void webserverLoop() {
    // 核心：每圈 loop 都要调用 handleClient()
    // 它会检查有没有浏览器请求进来，有则调用对应的 handleRoot 等函数
    // 如果不调用，网页会一直转圈/load 不出来
    server.handleClient();
}
