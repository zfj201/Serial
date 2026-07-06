#pragma once

// 模块 ID（#define 才能给 #if / -D 编译参数用）
#define MODULE_SERIAL_COMMAND  1
#define MODULE_REACTION_SPEED  2
#define MODULE_BREATHE_LED  3
#define MODULE_SWITCH_LED_MODE  4
#define MODULE_WIFI  5
#define MODULE_ESP_HOT_SPOT  6
#define MODULE_WEBSERVER  7
#define MODULE_WEB_BUTTON  8
#define MODULE_BLE  9
// 启动菜单超时内未选择时，默认运行的模块
#ifndef ACTIVE_MODULE
#define ACTIVE_MODULE MODULE_REACTION_SPEED
#endif

// 上电后等待串口选模块的毫秒数（0 = 不等待，直接用默认模块）
#ifndef MODULE_PICK_TIMEOUT_MS
#define MODULE_PICK_TIMEOUT_MS 5000
#endif
