#include <Arduino.h>
#include "config.h"
#include "module.h"

static const AppModule *activeModule = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  const int moduleId = pickModule(ACTIVE_MODULE, MODULE_PICK_TIMEOUT_MS);
  activeModule = findModule(moduleId);

  Serial.print("Running: ");
  Serial.println(activeModule->name);
  activeModule->setup();
}

void loop() {
  activeModule->loop();
}
