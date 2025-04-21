#ifndef IR_UTILS_H
#define IR_UTILS_H

#include <Arduino.h>

struct IRSignal {
  String name;
  uint8_t address;
  uint8_t command;
  uint32_t rawData;
};

void setupIR();
void handleIRSend();
void handleIRReceive();
void recordIRSignals();
void showSignalNames();
void printStoredSignals();
void playSignalByName(String name);

#endif
