#include <Wire.h>
#include <DHT11.h>
#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>

#define IR_RECEIVE_PIN 2
#define IR_SEND_PIN 4
#define SS_PIN 10
#define RST_PIN 9

DHT11 dht11(7);
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

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
bool isAuthenticated = false;
bool autoControl = false;

unsigned long lastUpdate = 0;
unsigned long authStartTime = 0;
unsigned long lastIRInteraction = 0;

const unsigned long AUTH_DURATION = 30000;
const unsigned long IR_CONTROL_TIMEOUT = 15000;

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AC Controller");
  delay(1000);
  lcd.clear();

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Place RFID card near reader...");
}

void loop() {
  if (millis() - lastUpdate > 2000) {
    int temperature = dht11.readTemperature();
    // Serial.println(temperature);
    delay(2000);

    // Auto control only if no IR input recently
    if (millis() - lastIRInteraction > IR_CONTROL_TIMEOUT) {
      if (temperature < 24) {
        acOn = true;
        targetTemp = 26;
        autoControl = true;
      } else if (temperature > 27) {
        acOn = false;
        targetTemp = 26;
        autoControl = true;
      } else {
        autoControl = false;
      }
    }

    updateLCD(temperature);
    lastUpdate = millis();
  }

  // Authentication timeout
  if (isAuthenticated && millis() - authStartTime > AUTH_DURATION) {
    isAuthenticated = false;
    Serial.println("Access expired. Scan RFID again.");
  }

  // RFID authentication
  if (!isAuthenticated) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      Serial.print("UID: ");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.println();

      isAuthenticated = true;
      authStartTime = millis();
      Serial.println("RFID authenticated. IR remote now active for 30 seconds.");
      mfrc522.PICC_HaltA();
    }
    return;
  }

  // Handle IR input
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

      // Register manual IR usage
      lastIRInteraction = millis();
      autoControl = false;

      if (storedSignals[i].name == "power") {
        acOn = !acOn;
      } else if (storedSignals[i].name == "up") {
        targetTemp++;
      } else if (storedSignals[i].name == "down") {
        targetTemp--;
      }

      IrSender.sendLG(addr, cmd, 0);
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
