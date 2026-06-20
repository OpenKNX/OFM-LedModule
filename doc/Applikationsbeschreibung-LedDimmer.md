# Applikationsbeschreibung LED-Dimmer

Die Applikation LED-Dimmer erlaubt die Verwaltung von Dimmkanälen innerhalb der ETS.

## Änderungshistorie

Im folgenden werden Änderungen an dem Dokument erfasst, damit man nicht immer das Gesamtdokument lesen muss, um Neuerungen zu erfahren.

* tbd

# **LED-Dimmer**

<!-- DOC HelpContext="Dokumentation" -->
Mit diesem Modul können LED-Dimmkanälen den verfügbaren Hardwarekanälen zugeordnet und parametrisiert werden.

<!-- DOCCONTENT 
https://github.com/OpenKNX/OFM-LedDimmer/blob/main/doc/Applikationsbeschreibung-LedDimmer.md
DOCCONTENT -->

## **Allgemein**

In der Titelzeile wird der Modulname und dessen Version ausgegeben. Diese Information ist für Support-Anfragen im OpenKNX-Forum relevant.

### **Hardware**

In diesem Bereich wird die verwendete Hardware ausgewählt.

<!-- DOC -->
#### **Hardware-Variante**

Basierend auf dieser Auswahl werden einige Konfigurationsoptionen ein- bzw. ausgeblendet.

<!-- DOC -->
### **Kanalzuordnung**

Hier erfolgt die Zuordnung von Dimmkanälen zu den verfügbaren Hardwarekanälen (A, B, C, ...).

Dazu wird zunächst der Typ ausgewählt. Aktuell werden unterstützt:

* EK - Einzelkanal
* TW - Tunable White
* RGB - Rot/Grün/Blau

Danach wird die Nummer des Dimmkanals vom gewählten Typ zugeordnet, die für den Hardwarekanal verwendet werden soll, wobei eine "0" deaktiviert meint.

Beispiel: Hardwarekanäle "A" und "B" sollen zusammen Tunable White LEDs dimmen, dann wird beiden Kanälen der Typ "TW - Tunable White" zugeordnet und der Dimmkanal bei beiden auf "1" gesetzt. Möchte man nun auch mit den Hardwarekanälen "C" und "D" andere Tunable White LEDs dimmen, so wählt man denselben Typ aus, jedoch weißt Dimmkanal "2" zu.

In der letzten Spalte wird noch die Funktion innerhalb des Dimmkanals zugeordnet. Bei Tunable White wäre das "Warmweiß" und "Kaltweiß", bei RGB-Kanäle entsprechend die drei Farben.

### **Weitere Einstellungen**

Je nach ausgewählter Hardware-Variante erscheinen hier zusätzliche Einstellungsmöglichkeiten.

<!-- DOC -->
#### **PWM-Frequenz**

<!-- DOC Skip="1" -->
Erscheint nur, wenn eine Hardware-Variante für Constant Voltage ausgewählt ist.

Hier kann die PWM-Frequenz zwischen 200 und 1200 Hz gewählt werden.

Höhere Frequenzen sind für das Auge (und Kameras) weniger sichtbar, niedrigere Frequenzen etwas effizienter (geringfügig weniger Strombedarf und Abwärme).

<!-- DOC -->
#### **Dimmart Constant Current**

<!-- DOC Skip="1" -->
Erscheint nur, wenn eine Hardware-Variante für Constant Current ausgewählt ist.

Hier kann die Art der Dimmung gewählt werden:

* Hybrides dimmen (analog/PWM): Hierbei wird oberhalb von 12,5 % Dimmwert analog gedimmt. Das bedeutet, dass der Constant Current entsprechend des Dimmwerts reduziert wird. Ist also beispielsweise ein Constant Current von 500 mA per Hardware-Widerstand für einen Kanal vorgegeben und der aktuell Dimmwert beträgt 50 %, so begrenzt der Dimmer den Constant Current auf lediglich 250 mA. Die Auflösung beträgt hierbei 8 Bit (= 256 Werte). Da die Reduktion des Constant Current nicht unbegrenzt funktioniert, wird unterhalb von 12,5 % Dimmwert immer PWM gedimmt.

* Reines PWM dimmen: Grundsätzlich ist die Empfehlung hybrides dimmen zu verwenden, um die Vorteile einer Konstantstromquelle ohne sichtbares "Flackern" auszunutzen. Sollte in bestimmten Situation jedoch eine Auflösung von 8 Bit nicht ausreichen, so kann der Dimmer hier auf reines PWM dimmen umgeschaltet werden.
