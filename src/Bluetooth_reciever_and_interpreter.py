import asyncio                          # Ermöglicht asynchrone Programmierung (gleichzeitiges Warten auf mehrere Ereignisse)
from bleak import BleakClient, BleakScanner  # Bibliothek für Bluetooth Low Energy (BLE) unter Python
import struct                           # Zum Umwandeln binärer Daten in Python-Datentypen
from datetime import datetime           # Für Zeitstempel bei Drehungen
from collections import deque           # Effizienter Speicher für die letzten Messungen

# UUIDs (eindeutige Adressen) des BLE-Service und der Characteristic
# Stimmen mit den Werten im Arduino-Programm überein
SERVICE_UUID = "e53e54ad-7cb0-476d-8980-2c5c27baf23b"
CHAR_UUID = "43d666ba-f270-4f96-8c88-eeb13565f6d8"


# Speicher für die letzten vier empfangenen Werte (zur Filterung)
last_turns = deque(maxlen=4)

# Zeitpunkt der letzten bestätigten Drehung (um Mehrfachmeldungen zu vermeiden)
last_turn_time = None

# Zuordnung von Zahlenwerten zu Farben der Megaminx-Seiten
color_dict = {
    0: "rote", 1: "grüne", 2: "schwarze", 3: "rote", 4: "braune", 5: "weisse",
    6: "blaue", 7: "gelbe", 8: "dunkelgraue", 9: "dunkelblaue", 10: "hellgrüne", 
    11: "lila", 12: "orange"
}

# Log aller Drehungen (Zeit, Seite, Drehrichtung)
turn_log = []

# Wird bei Empfang einer neuen Drehung automatisch aufgerufen. Prüft die Daten und fragt bei Bedarf nach einer Bestätigung.
def notification_handler_with_filter(sender, data):
    global last_turn_time, last_turns

    # Umwandlung der empfangenen Byte-Daten in eine ganze Zahl
    recieved_turn = struct.unpack('<B', data)[0]

    # Prüfen, ob der gleiche Wert mindestens dreimal hintereinander empfangen wurde als Filter gegen Fehlmessungen und ob seit der letzten bestätigten Drehung mindestens 1 Sekunde vergangen ist
    if sum(1 for turn in last_turns if turn == recieved_turn) >= 3 and (last_turn_time is None or (datetime.now() - last_turn_time).total_seconds() > 1):

        # Zeitpunkt der aktuellen Drehung speichern     
        turn_time = datetime.now()

        # Drehrichtung anhand des Zahlenwerts bestimmen: Werte 1-12 = Uhrzeigersinn, 13-24 = Gegenuhrzeigersinn (ID + 12) und Anschliessend Bestätigung vom Benutzer einholen und in das Log aufnehmen
        if recieved_turn > 12:
            confirm = input(f"Die {color_dict.get(recieved_turn-12)} Seite wurde im Gegenuhrzeigersinn gedreht. Ist das korrekt? (j/n): ")
            turn_log.append((turn_time,color_dict.get(recieved_turn-12), "Gegenuhrzeigersinn")) 
        else:
            confirm = input(f"Die {color_dict.get(recieved_turn)} Seite wurde im Uhrzeigersinn gedreht.Ist das korrekt? (j/n): ")
            turn_log.append((turn_time,color_dict.get(recieved_turn), "Uhrzeigersinn"))

         # Wenn der Benutzer die Drehung als falsch markiert, wird sie ignoriert
        if confirm.lower() == 'n':
            print("Falsche Drehung erkannt. Ignoriere diese Benachrichtigung.")
            turn_log.pop()  # Letzte Drehung entfernen, da sie als falsch markiert wurde

        # Zeitpunkt der letzten bestätigten Drehung speichern
        last_turn_time = datetime.now()

    # Neue Messung zur Verlaufsliste hinzufügen
    last_turns.append(recieved_turn)



# Hauptfunktion: sucht nach dem Megaminx-Sensor, verbindet sich und empfängt Drehdaten.
async def main():
    # Scan nach verfügbaren BLE-Geräten in der Umgebung
    devices = await BleakScanner.discover()

    for d in devices:
        # Prüfen, ob der gefundene Gerätename mit dem Namen aus dem Arduino-Code übereinstimmt
        if "Smart_Megaminx" == d.name:
            print(f"Gefundenes Gerät: {d.name}, {d.address}")

            # Verbindung zum gefundenen Gerät herstellen und Benachichtigungenhandler registrieren
            async with BleakClient(d.address) as client:
                await client.start_notify(CHAR_UUID, notification_handler_with_filter)
                print("Verbunden, warte auf Drehdaten...")

                # 5 Minuten lang auf Benachrichtigungen warten (300 Sekunden)
                await asyncio.sleep(300)

                # Übersicht aller erkannten Drehungen ausgeben
                print(f"Erkannte Drehungen:\n{turn_log}")

                # Abmelden von Benachrichtigungen
                await client.stop_notify(CHAR_UUID)
            break
    else:
        print("Kein Sensor gefunden.")


# Einstiegspunkt: Startet das asynchrone Programm
asyncio.run(main())
