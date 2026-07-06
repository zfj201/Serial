#include <Arduino.h>
#include "hardware.h"
#include "serial_command.h"

void serialCommandSetup() {
  pinMode(PIN_LED1, OUTPUT);
  Serial.println("Hello from ESP32-C3!");
  Serial.println("Commands: led_on / led_off / led_toggle");
}

void serialCommandLoop() {
  if (!Serial.available()) {
    return;
  }

  String message = Serial.readStringUntil('\n');
  message.trim();
  if (message.length() == 0) {
    return;
  }

  Serial.println("Received: " + message);

  if (message == "led_on") {
    digitalWrite(PIN_LED1, HIGH);
  } else if (message == "led_off") {
    digitalWrite(PIN_LED1, LOW);
  } else if (message == "led_toggle") {
    digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));
  } else {
    Serial.println("Invalid message");
  }
}
