#pragma once

#include <stddef.h>

struct AppModule {
  int id;
  const char *name;
  const char *description;
  void (*setup)();
  void (*loop)();
};

const AppModule *findModule(int id);
const AppModule *moduleByIndex(size_t index);
size_t moduleCount();
void printModuleMenu();
int pickModule(int defaultId, unsigned long timeoutMs);
