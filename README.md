# Smart_Megaminx
Dieses Projekt wurde im Rahmen eines Schulprojekts durchgeführt, weshalb die Dateien, die für den Nachbau notwendig wären, hier nicht bereitgestellt werden. Sollten Personen daran interessiert sein, einen solchen Megaminx selbst zu konstruieren, können sie mich über GitHub kontaktieren. Im Folgenden wird die Verwendung des Würfels beschrieben.

## Vorbereitung
Um den Würfel zu verwenden, wird das Python-Programm aus dem src-Ordner benötigt, und die Bibliothek bleak muss installiert sein. Des Weiteren muss sichergestellt werden, dass das Programm in einer Umgebung ausgeführt wird, die über eine Konsole verfügt.

## Anwendung
Um den Würfel zu nutzen, muss der Mittelstein der schwarzen Seite einmal gedrückt werden, um den Würfel einzuschalten. Daraufhin leuchtet ein blaues LED-Lämpchen unterhalb der schwarzen Seite. Sobald dieses Lämpchen leuchtet, kann das Python-Programm gestartet werden. Nachdem eine Verbindung zustande gekommen ist, meldet dies das Python-Programm, und nun kann gedreht werden. Sobald die Elektronik eine Drehung feststellt, fordert sie den Nutzer auf, diese zu bestätigen. Hierbei ist zu beachten, dass die Drehung zunächst vollständig beendet werden sollte, bevor eine Bestätigung erfolgt, da sonst die Gefahr besteht, dass die Drehung mehrfach erfasst wird.

## Ausschalten 
Um den Würfel auszuschalten, muss der Mittelstein der schwarzen  Seite zweimal kurz hintereinander gedrückt werden.
