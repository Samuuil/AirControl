#include <Wire.h>
#include <DHT11.h>
#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>

#define IR_RECEIVE_PIN 2
#define IR_SEND_PIN 4

DHT11 dht11(8);

LiquidCrystal_I2C lcd(0x27, 16, 2);

struct IRSignal {
  String name;
  uint8_t address;
  uint8_t command;
  uint32_t rawData;
};

IRSignal storedSignals[3] = {
  {"power", 0x04, 0x08, 0xF708FB04},
  {"up",    0x04, 0x00, 0xFF00FB04},
  {"down",  0x04, 0x01, 0xFE01FB04}
};

int targetTemp = 24;
bool acOn = false;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AC Controller");
  delay(1000);
  lcd.clear();
}

void loop() {
  if (millis() - lastUpdate > 1000) {
    int temperature = dht11.readTemperature();
    updateLCD(temperature);
    lastUpdate = millis();
  }

  if (IrReceiver.decode()) {
    handleIR();
    IrReceiver.resume();
  }
}

void handleIR() {
  if (IrReceiver.decodedIRData.protocol == UNKNOWN) return;

  uint8_t addr = IrReceiver.decodedIRData.address;
  uint8_t cmd = IrReceiver.decodedIRData.command;
  uint32_t raw = IrReceiver.decodedIRData.decodedRawData;

  Serial.print("IR Received: Addr=");
  Serial.print(addr, HEX);
  Serial.print(" Cmd=");
  Serial.print(cmd, HEX);
  Serial.print(" Raw=");
  Serial.println(raw, HEX);

  for (int i = 0; i < 3; i++) {
    if (storedSignals[i].address == addr && storedSignals[i].command == cmd) {
      Serial.print("Matched: ");
      Serial.println(storedSignals[i].name);

      if (storedSignals[i].name == "power") {
        acOn = !acOn;
      } else if (storedSignals[i].name == "up") {
        targetTemp++;
      } else if (storedSignals[i].name == "down") {
        targetTemp--;
      }

      IrSender.sendNEC(addr, cmd, 0);
      return;
    }
  }

  Serial.println("No match found.");
}

void updateLCD(int currentTemp) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(currentTemp);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Set: ");
  lcd.print(targetTemp);
  lcd.print((char)223);
  lcd.print("C ");

  lcd.print(acOn ? "ON " : "OFF");
}
