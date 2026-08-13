// 本模块：先连家里路由器 WiFi，再在本机 80 端口提供网页访问
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>          // ESP32 内置 HTTP 库，注意用尖括号
#include "webserver_module.h"   // 本地头文件故意不用 webserver.h，避免 macOS 上遮蔽 <WebServer.h>
#include "hardware.h"

// static const char *WIFI_SSID = "2801";
// static const char *WIFI_PASS = "18367168360";
// static const char *WIFI_SSID = "MolarData-Office";
// static const char *WIFI_PASS = "molaroffice2023";
static const char *WIFI_SSID = "OPPO K13 Turbo Pro 5G C216";
static const char *WIFI_PASS = "oppooppo";
static const unsigned long CONNECT_TIMEOUT_MS = 15000;

struct DeviceInfo {
    String name;
    String ip;
    String mac;
};
struct DeviceMetrics {
    uint32_t uptimeMs;
    int32_t rssiDbm;
    uint32_t freeHeapBytes;
    float chipTempC;
};

struct DeviceState {
    bool streaming;
    uint32_t counter;
    uint32_t intervalMs;
};

static DeviceState deviceState{
    false,  // streaming
    0,      // counter
    1000    // intervalMs
};

static unsigned long lastCounterMs = 0;
static DeviceInfo readDeviceInfo() {
    DeviceInfo info;
    info.name = "esp32-c3-lab";
    info.ip = WiFi.localIP().toString();
    info.mac = WiFi.macAddress();
    return info;
}
static DeviceMetrics readDeviceMetrics() {
    DeviceMetrics metrics;
    metrics.uptimeMs = millis();
    metrics.rssiDbm = WiFi.RSSI();
    metrics.freeHeapBytes = ESP.getFreeHeap();
    metrics.chipTempC = temperatureRead();
    return metrics;
}

static String deviceInfoToJson(const DeviceInfo &info) {
    String json;
    json.reserve(128);

    json += "{";
    json += "\"name\":\"" + info.name + "\",";
    json += "\"ip\":\"" + info.ip + "\",";
    json += "\"mac\":\"" + info.mac + "\"";
    json += "}";

    return json;
}
static String deviceMetricsToJson(const DeviceMetrics &metrics) {
    String json;
    json.reserve(160);

    json += "{";
    json += "\"uptime_ms\":" + String(metrics.uptimeMs) + ",";
    json += "\"rssi_dbm\":" + String(metrics.rssiDbm) + ",";
    json += "\"free_heap_bytes\":" + String(metrics.freeHeapBytes) + ",";
    json += "\"chip_temp_c\":" + String(metrics.chipTempC, 1);
    json += "}";

    return json;
}
static String deviceStateToJson(const DeviceState &state) {
    String json;
    json.reserve(100);

    json += "{";
    json += "\"streaming\":";
    json += state.streaming ? "true" : "false";
    json += ",";
    json += "\"counter\":" + String(state.counter) + ",";
    json += "\"interval_ms\":" + String(state.intervalMs);
    json += "}";

    return json;
}
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
static void sendDeviceState() {
    server.send(
        200,
        "application/json; charset=utf-8",
        deviceStateToJson(deviceState)
    );
}

// 路由回调：有人访问 "/" 时，WebServer 库会自动调用这个函数
// 注意：这不是 loop 里手动调的，而是 handleClient() 收到请求后触发的
static void handleRoot() {
    const char *page = R"HTML(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32-C3 虚拟设备</title>
  <style>
    body {
      max-width: 720px;
      margin: 40px auto;
      padding: 0 16px;
      font-family: sans-serif;
    }

    button, input {
      margin: 4px;
      padding: 8px 12px;
    }

    #message {
      color: #555;
    }

    .charts {
      display: grid;
      gap: 12px;
      margin: 16px 0 24px;
    }

    .chart-card {
      border: 1px solid #ddd;
      border-radius: 8px;
      padding: 12px;
    }

    .chart-card h3 {
      margin: 0 0 4px;
      font-size: 15px;
    }

    .chart-card .now {
      color: #333;
      font-size: 18px;
      margin-bottom: 8px;
    }

    .chart-card canvas {
      width: 100%;
      height: 140px;
      display: block;
      background: #fafafa;
    }
  </style>
</head>
<body>
  <h1>ESP32-C3 虚拟设备</h1>

  <p>运行状态：<strong id="streaming">未知</strong></p>
  <p>计数器：<strong id="counter">0</strong></p>
  <p>计数间隔：<strong id="interval">0</strong> ms</p>

  <hr>

    <h2>设备指标</h2>

    <p>运行时间：<strong id="uptime">--</strong></p>

    <div class="charts">
      <div class="chart-card">
        <h3>Wi-Fi 信号</h3>
        <div class="now"><strong id="rssi">--</strong> dBm</div>
        <canvas id="rssiChart"></canvas>
      </div>
      <div class="chart-card">
        <h3>空闲堆内存</h3>
        <div class="now"><strong id="freeHeap">--</strong> bytes</div>
        <canvas id="heapChart"></canvas>
      </div>
      <div class="chart-card">
        <h3>芯片温度</h3>
        <div class="now"><strong id="chipTemp">--</strong> °C</div>
        <canvas id="tempChart"></canvas>
      </div>
    </div>

    <p>
    HTTP 连接：
    <strong id="connectionStatus">正在连接</strong>
    </p>

    <p>
    轮询次数：
    <strong id="pollCount">0</strong>
    </p>
  <button onclick="postCommand('/api/stream/start')">开始</button>
  <button onclick="postCommand('/api/stream/stop')">停止</button>
  <button onclick="postCommand('/api/counter/reset')">清零</button>

  <div>
    <input id="intervalInput"
           type="number"
           min="1"
           max="10000"
           value="1000">
    <button onclick="setIntervalMs()">修改间隔</button>
  </div>

  <p id="message">准备就绪</p>

  <script>
    function renderState(state) {
      document.getElementById('streaming').textContent =
        state.streaming ? '运行中' : '已停止';

      document.getElementById('counter').textContent =
        state.counter;

      document.getElementById('interval').textContent =
        state.interval_ms;
    }
    const HISTORY_LIMIT = 60;

    function createChart(canvasId, color, yMin, yMax) {
      return {
        canvas: document.getElementById(canvasId),
        color: color,
        yMin: yMin,
        yMax: yMax,
        values: []
      };
    }

    const rssiChart = createChart('rssiChart', '#2563eb', -90, -20);
    const heapChart = createChart('heapChart', '#059669', 0, 320000);
    const tempChart = createChart('tempChart', '#dc2626', 20, 60);

    function pushChartValue(chart, value) {
      if (!Number.isFinite(value)) {
        return;
      }
      chart.values.push(value);
      if (chart.values.length > HISTORY_LIMIT) {
        chart.values.shift();
      }
      drawChart(chart);
    }

    function drawChart(chart) {
      const canvas = chart.canvas;
      const rect = canvas.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      const width = Math.max(rect.width, 1);
      const height = Math.max(rect.height, 1);

      canvas.width = width * dpr;
      canvas.height = height * dpr;

      const ctx = canvas.getContext('2d');
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, width, height);

      const padL = 42;
      const padR = 8;
      const padT = 8;
      const padB = 8;
      const plotW = width - padL - padR;
      const plotH = height - padT - padB;
      let yMin = chart.yMin;
      let yMax = chart.yMax;

      if (chart.values.length > 0) {
        const dataMin = Math.min.apply(null, chart.values);
        const dataMax = Math.max.apply(null, chart.values);
        yMin = Math.min(yMin, dataMin);
        yMax = Math.max(yMax, dataMax);
      }

      if (yMax === yMin) {
        yMax = yMin + 1;
      }

      ctx.strokeStyle = '#eee';
      ctx.fillStyle = '#888';
      ctx.font = '11px sans-serif';
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';

      for (let i = 0; i <= 4; ++i) {
        const ratio = i / 4;
        const y = padT + plotH * ratio;
        const tick = yMax - (yMax - yMin) * ratio;
        ctx.beginPath();
        ctx.moveTo(padL, y);
        ctx.lineTo(width - padR, y);
        ctx.stroke();
        ctx.fillText(Math.round(tick), padL - 4, y);
      }

      if (chart.values.length < 2) {
        return;
      }

      ctx.beginPath();
      ctx.strokeStyle = chart.color;
      ctx.lineWidth = 2;

      for (let i = 0; i < chart.values.length; ++i) {
        const x = padL + (plotW * i) / (HISTORY_LIMIT - 1);
        const y = padT + plotH * (1 - (chart.values[i] - yMin) / (yMax - yMin));
        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      }
      ctx.stroke();
    }

    function renderMetrics(metrics) {
        document.getElementById('uptime').textContent =
            formatUptime(metrics.uptime_ms);

        document.getElementById('rssi').textContent =
            metrics.rssi_dbm;

        document.getElementById('freeHeap').textContent =
            metrics.free_heap_bytes;

        document.getElementById('chipTemp').textContent =
            metrics.chip_temp_c;

        pushChartValue(rssiChart, metrics.rssi_dbm);
        pushChartValue(heapChart, metrics.free_heap_bytes);
        pushChartValue(tempChart, metrics.chip_temp_c);
    }

    function formatUptime(milliseconds) {
    const totalSeconds = Math.floor(milliseconds / 1000);

    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;

    return `${hours}时 ${minutes}分 ${seconds}秒`;
    }
    async function request(path, options = {}) {
      const response = await fetch(path, options);
      const data = await response.json();

      if (!response.ok) {
        throw new Error(data.error || '请求失败');
      }

      return data;
    }

    async function loadState() {
      try {
        const state = await request('/api/state');
        renderState(state);
      } catch (error) {
        showMessage(error.message);
      }
    }

    async function postCommand(path) {
      try {
        const state = await request(path, {
          method: 'POST'
        });

        renderState(state);
        showMessage('操作成功');
      } catch (error) {
        showMessage(error.message);
      }
    }

    async function setIntervalMs() {
      const value =
        document.getElementById('intervalInput').value;

      await postCommand(
        '/api/interval?value=' + encodeURIComponent(value)
      );
    }

    function showMessage(message) {
      document.getElementById('message').textContent = message;
    }

    const POLL_INTERVAL_MS = 1000;

let pollCount = 0;
let pollTimer = null;

async function pollDashboard() {
  try {
    const [state, metrics] = await Promise.all([
      request('/api/state'),
      request('/api/metrics')
    ]);

    renderState(state);
    renderMetrics(metrics);

    pollCount++;

    document.getElementById('pollCount').textContent =
      pollCount;

    document.getElementById('connectionStatus').textContent =
      '正常';

    document.getElementById('connectionStatus').style.color =
      'green';
  } catch (error) {
    document.getElementById('connectionStatus').textContent =
      '连接失败';

    document.getElementById('connectionStatus').style.color =
      'red';

    showMessage(error.message);
  } finally {
    pollTimer = setTimeout(
      pollDashboard,
      POLL_INTERVAL_MS
    );
  }
}

    pollDashboard();
  </script>
</body>
</html>
)HTML";

    server.send(200, "text/html; charset=utf-8", page);
}

// 访问了没注册的路径（如 /abc）时触发
static void handleNotFound() {
    server.send(404, "text/plain", "Not Found");
}

static void handleGetInfo() {
    DeviceInfo info = readDeviceInfo();
    String json = deviceInfoToJson(info);

    server.send(200, "application/json; charset=utf-8", json);
}

static void handleGetMetrics() {
    DeviceMetrics metrics = readDeviceMetrics();
    String json = deviceMetricsToJson(metrics);
    server.send(200, "application/json; charset=utf-8", json);
}

static void handleGetState() {
    sendDeviceState();
}
static void handleStartStream() {
    deviceState.streaming = true;
    lastCounterMs = millis();

    sendDeviceState();
}
static void handleStopStream() {
    deviceState.streaming = false;

    sendDeviceState();
}
static void handleResetCounter() {
    deviceState.counter = 0;
    lastCounterMs = millis();

    sendDeviceState();
}
static void handleSetInterval() {
    if (!server.hasArg("value")) {
        server.send(
            400,
            "application/json; charset=utf-8",
            "{\"error\":\"missing value\"}"
        );
        return;
    }

    long value = server.arg("value").toInt();

    if (value < 1 || value > 10000) {
        server.send(
            400,
            "application/json; charset=utf-8",
            "{\"error\":\"value must be between 1 and 10000\"}"
        );
        return;
    }

    deviceState.intervalMs = static_cast<uint32_t>(value);
    lastCounterMs = millis();

    sendDeviceState();
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
    // server.on("/on", handleOn);
    // server.on("/off", handleOff);
    // server.on("/status", handleStatus);
    server.on("/api/info", HTTP_GET, handleGetInfo);
    server.on("/api/metrics", HTTP_GET, handleGetMetrics);
    server.on("/api/state", HTTP_GET, handleGetState);

    server.on("/api/stream/start", HTTP_POST, handleStartStream);
    server.on("/api/stream/stop", HTTP_POST, handleStopStream);
    server.on("/api/counter/reset", HTTP_POST, handleResetCounter);
    server.on("/api/interval", HTTP_POST, handleSetInterval);
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

    if (!deviceState.streaming) {
        return;
    }

    unsigned long now = millis();

    if (now - lastCounterMs >= deviceState.intervalMs) {
        lastCounterMs = now;
        deviceState.counter++;
    }
}
