#include <Arduino.h>
#include "hardware.h"
#include "reaction_speed.h"

static bool isBootPressed() {
  return digitalRead(PIN_BOOT) == LOW;
}

static void setLeds(bool on) {
  digitalWrite(PIN_LED1, on ? HIGH : LOW);
  digitalWrite(PIN_LED2, on ? HIGH : LOW);
}

static void waitForRelease() {
  while (isBootPressed()) {
    delay(10);
  }
}

void reactionSpeedSetup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  randomSeed(micros());

  setLeds(false);
  Serial.println("Reaction Speed Game");
  Serial.println("Wait for the LED, then press BOOT!");
}

void reactionSpeedLoop() {
  setLeds(false);
  waitForRelease();

  int waitMs = random(2000, 5001);
  Serial.println("Wait...");

  unsigned long waitStart = millis();
  while (millis() - waitStart < (unsigned long)waitMs) {
    if (isBootPressed()) {
      Serial.println("Too early! Wait for the LED.");
      waitForRelease();
      delay(1000);
      return;
    }
  }

  unsigned long goTime = millis();
  setLeds(true);

  while (!isBootPressed()) {
    delay(1);
  }

  unsigned long reactionMs = millis() - goTime;
  setLeds(false);

  Serial.print("Reaction time: ");
  Serial.print(reactionMs);
  Serial.println(" ms");

  waitForRelease();
  delay(2000);
}
