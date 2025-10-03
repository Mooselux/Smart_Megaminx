#include <Wire.h>              // Bibliothek für I²C-Kommunikation (für MCP23017)
#include <Adafruit_MCP23X17.h> // Bibliothek für MCP23017 I/O-Expander
// Hinweis: In dieser Bibliothek müssen digitalRead / digitalWrite zu digitalReadAda / digitalWriteAda umbenannt werden,
// damit es keine Konflikte mit der ESP32-Arduino-Implementierung gibt.
#include <ArduinoBLE.h>        // Bibliothek für BLE-Funktionalität


// BLE-Service zur Übertragung der Drehdaten
BLEService megaminxRotationService("e53e54ad-7cb0-476d-8980-2c5c27baf23b");

// BLE-Characteristic, über die eine einzelne Messung (1–24) gesendet wird
BLEUnsignedCharCharacteristic faceTurnCharacteristic("43d666ba-f270-4f96-8c88-eeb13565f6d8", BLERead | BLENotify);

unsigned long lastUpdate = 0;  // Zeitstempel zur Steuerung, wie oft Messdaten gesendet werden

#define I2C_ADDR_1 0x20  // I²C-Adresse des ersten MCP23X17
#define I2C_ADDR_2 0x21  // I²C-Adresse des zweiten MCP23X17

// Zwei I/O-Expander-Objekte für die 12 Encoder
Adafruit_MCP23X17 mcp[2];

// Datenstruktur, die alle relevanten Informationen eines Encoders speichert
struct Encoder {
  uint8_t id;            // Eindeutige ID des Encoders (1–12)
  uint8_t module_id;         // Welcher MCP-Expander genutzt wird (0 oder 1)
  uint8_t pinA;          // Pin A des Encoders
  uint8_t pinB;          // Pin B des Encoders
  uint8_t lastRead;  // Letzter erfasster Pinzustand zur Drehrichtungserkennung
  };


// Array aller 12 Encoderinstanzen
Encoder encoders[] = {
  {1, 0, 0, 1, 3},
  {2, 0, 2, 3, 3},
  {3, 0, 4, 5, 3},
  {4, 0, 8, 9, 3},
  {5, 0, 10, 11, 3},
  {6, 0, 12, 13, 3},
  {7, 1, 0, 1, 3},
  {8, 1, 2, 3, 3},
  {9, 1, 4, 5, 3},
  {10, 1, 8, 9, 3},
  {11, 1, 10, 11, 3},
  {12, 1, 12, 13, 3},
};


// Prüft alle Encoder auf eine Zustandsänderung und gibt ein Ereignis zurück:
//   - 1–12: Drehung im Uhrzeigersinn
//   - 13–24: Drehung gegen den Uhrzeigersinn (ID + 12)
//   - 25: Ungültige Signalfolge erkannt
//   - 26: Kein Encoder hat sich verändert
uint8_t checkEncoders(){
  for (Encoder& e : encoders){
    // Kombiniert die beiden Eingangssignale zu einem 2-Bit-Wert (z. B. 00, 01, 10 oder 11)
    uint8_t latestRead = (mcp[e.module_id].digitalReadAda(e.pinA)<<1) | mcp[e.module_id].digitalReadAda(e.pinB); 

    // Nur wenn sich der Zustand seit dem letzten Lesen geändert hat, wird die Drehrichtung bestimmt
    if (latestRead != e.lastRead) {

      //Überprüfung ob die  Zustandsfolge einer Drehung im Gegenuhrzeigersinn entspricht
      if ((e.lastRead == 0b00 && latestRead == 0b01) ||
          (e.lastRead == 0b01 && latestRead == 0b11) ||
          (e.lastRead == 0b11 && latestRead == 0b10) ||
          (e.lastRead == 0b10 && latestRead == 0b00)) { 
        e.lastRead = latestRead;
        return(e.id + 12);
      }
      //Überprüfung ob die  Zustandsfolge einer Drehung im Uhrzeigersinn entspricht
       else if ((e.lastRead == 0b00 && latestRead == 0b10) ||
                (e.lastRead == 0b10 && latestRead == 0b11) ||
                (e.lastRead == 0b11 && latestRead == 0b01) ||
                (e.lastRead == 0b01 && latestRead == 0b00)) {
        e.lastRead = latestRead;
        return(e.id);
      } 
      // Signaländerung war nicht eindeutig auswertbar
      else {
        e.lastRead = latestRead;
        return(25);
      }
    }
  }
  // Keine Änderung erkannt
  return(26);
}


// Vorbereitungsprogramm: Setzt sowohl die BLE als auch die I²C-Verbindung auf und richtet die Encoder ein
void setup() {
  delay(1000); // Kurze Verzögerung für stabilen Start

  // --- Einrichtung der BLE-Kommunikation ---
  BLE.begin();
  BLE.setLocalName("Smart_Megaminx");          // Anzeigename des Geräts
  BLE.setAdvertisedService(megaminxRotationService);
  megaminxRotationService.addCharacteristic(faceTurnCharacteristic);
  BLE.addService(megaminxRotationService);
  faceTurnCharacteristic.writeValue(0);        // Anfangswert der Drehdaten
  BLE.advertise();                             // Gerät für andere sichtbar machen


  // --- Initialisierung der I²C-I/O-Expander ---
  mcp[0].begin_I2C(I2C_ADDR_1);
  mcp[1].begin_I2C(I2C_ADDR_2);


  //----------Konfigurieren der einzelnen Pins der Encoder----------
  for (Encoder& e : encoders){
    mcp[e.module_id].pinModeAda(e.pinA, INPUT);
    mcp[e.module_id].pinModeAda(e.pinB, INPUT);
    e.lastRead = (mcp[e.module_id].digitalReadAda(e.pinA)<<1) | mcp[e.module_id].digitalReadAda(e.pinB);
  }
}

// Hauptprogramm: Überwacht Encoder und sendet bei Bewegung die entsprechende ID per BLE
void loop() {
  BLEDevice central = BLE.central();  // Prüft, ob ein Gerät verbunden ist

  if (central) {
    while (central.connected()) {
      unsigned long now = millis();

      // Nur alle 10 ms nach Änderungen suchen, um Rechenzeit zu sparen
      if (now - lastUpdate > 10) {
        lastUpdate = now;
        uint8_t turnRead = checkEncoders();  // Drehrichtung prüfen

        // Nur gültige Drehungen übertragen
        if (turnRead != 25 && turnRead != 26) {
          faceTurnCharacteristic.writeValue(turnRead);  // ID an das zentrale Gerät senden
        }
      }
    }
  }
}
