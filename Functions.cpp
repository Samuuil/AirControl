#include "Functions.h"
#include <IRremote.hpp>

#define IR_SEND_PIN 4
#define IR_RECEIVE_PIN 2

IRSignal storedSignals[3];

void setupIR() {
  Serial.begin(115200);
  while (!Serial);

  IrSender.begin(IR_SEND_PIN);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.println(F("IR Sender & Receiver initialized."));

  recordIRSignals();
  showSignalNames();
  printStoredSignals();
}

void handleIRSend() {
  static unsigned long lastSendTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastSendTime >= 2000) {
    Serial.println(F("Sending sample LG command..."));
    IrSender.sendLG(0x20DF10EF, 32); // Example command
    lastSendTime = currentTime;
  }
}

void handleIRReceive() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      Serial.println(F("Received unknown or noisy signal."));
      IrReceiver.printIRResultRawFormatted(&Serial, true);
    } else {
      IrReceiver.printIRResultShort(&Serial);
      IrReceiver.printIRSendUsage(&Serial);
    }
    IrReceiver.resume();
    Serial.println();
  }
}

void recordIRSignals() {
  int count = 0;
  while (count < 3) {
    Serial.print(F("Place the remote and send signal #"));
    Serial.print(count + 1);
    Serial.println(F(" (press a button or send a signal)"));

    while (!IrReceiver.decode()) {
      // Wait for a signal
    }

    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      Serial.print(F("Enter a name for signal "));
      Serial.print(count + 1);
      Serial.println(F(" (max 20 characters):"));

      String signalName = "";
      while (Serial.available() == 0);
      signalName = Serial.readString();
      signalName.trim();

      storedSignals[count].name = signalName;
      storedSignals[count].address = IrReceiver.decodedIRData.address;
      storedSignals[count].command = IrReceiver.decodedIRData.command;
      storedSignals[count].rawData = IrReceiver.decodedIRData.decodedRawData;

      Serial.print(F("Stored signal: "));
      Serial.println(storedSignals[count].name);

      IrReceiver.printIRResultShort(&Serial);

      count++;
    } else {
      Serial.println(F("Received an unknown signal. Please try again."));
    }

    IrReceiver.resume();
  }

  Serial.println(F("Finished recording 3 signals."));
}

void showSignalNames() {
  Serial.println(F("\nAvailable Signals:"));
  for (int i = 0; i < 3; i++) {
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.println(storedSignals[i].name);
  }
}

void printStoredSignals() {
  Serial.println(F("\nStored IR Signals:"));
  for (int i = 0; i < 3; i++) {
    Serial.print(F("Signal #"));
    Serial.print(i + 1);
    Serial.print(F(" - Name: "));
    Serial.println(storedSignals[i].name);

    Serial.print(F("Address: "));
    Serial.println(storedSignals[i].address, HEX);

    Serial.print(F("Command: "));
    Serial.println(storedSignals[i].command, HEX);

    Serial.print(F("Raw Data: "));
    Serial.println(storedSignals[i].rawData, HEX);
  }
}

void playSignalByName(String name) {
  for (int i = 0; i < 3; i++) {
    if (storedSignals[i].name == name) {
      Serial.print(F("Sending signal: "));
      Serial.println(name);
      IrSender.sendLG(storedSignals[i].rawData, 32);
      return;
    }
  }
  Serial.println(F("Signal not found."));
}
