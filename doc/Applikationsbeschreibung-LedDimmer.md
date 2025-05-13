# Applikationsbeschreibung LED-Dimmer

Die Applikation LED-Dimmer erlaubt die Verwaltung von Dimmkanälen innerhalb der ETS.

## Änderungshistorie

Im folgenden werden Änderungen an dem Dokument erfasst, damit man nicht immer das Gesamtdokument lesen muss, um Neuerungen zu erfahren.

13.05.2025: Firmware 0.1.0, Applikation 0.1

* Initiales Release.

## **Verwendete Module**

Die Zutrittskontrolle verwendet weitere OpenKNX-Module, die alle ihre eigene Dokumentation besitzen. Im folgenden werden die Module und die Verweise auf deren Dokumentation aufgelistet.

### **OpenKNX**

Dies ist eine Seite mit allgemeinen Parametern, die unter [Applikationsbeschreibung-Common](https://github.com/OpenKNX/OGM-Common/blob/v1/doc/Applikationsbeschreibung-Common.md) beschrieben sind. 

### **Konfigurationstransfer**

Der Konfigurationstransfer erlaubt einen

* Export von Konfigurationen von OpenKNX-Modulen und deren Kanälen
* Import von Konfigurationen von OpenKNX-Modulen und deren Kanälen
* Kopieren der Konfiguration von einem OpenKNX-Modulkanal auf einen anderen
* Zurücksetzen der Konfiguration eines OpenKNX-Modulkanals auf Standardwerte

Die Funktionen vom Konfigurationstranfer-Modul sind unter [Applikationsbeschreibung-ConfigTransfer](https://github.com/OpenKNX/OFM-ConfigTransfer/blob/v1/doc/Applikationsbeschreibung-ConfigTransfer.md) beschrieben.

### **Schaltaktor**

Die Funktionen vom Schaltaktor-Modul sind unter [Applikationsbeschreibung-Schaltaktor](https://github.com/OpenKNX/OFM-SwitchActuator/blob/main/doc/Applikationbeschreibung-Schaltaktor.md) beschrieben.

### **Logiken**

Wie die meisten OpenKNX-Applikationen enthält auch diese Applikation ein Logikmodul.

Die Funktionen des Logikmoduls sind unter [Applikationsbeschreibung-Logik](https://github.com/OpenKNX/OFM-LogicModule/blob/v1/doc/Applikationsbeschreibung-Logik.md) beschrieben.

### **Virtuelle Taster**

Es werden auch virtuelle Taster von der Applikation angeboten. Mit der Nutzung der Binäreingänge oder der Touch-Platine (als Erweiterung) - können es auch echte Taster werden.

Die Funktionen des Tastermoduls sind unter [Applikationsbeschreibung-Taster](https://github.com/OpenKNX/OFM-VirtualButton/blob/v1/doc/Applikationsbeschreibung-Taster.md) beschrieben.

### **Binäreingänge**

Diese Applikation unterstützt auch Binäreingänge.

Die Funktionen der Binäreingänge sind unter [Applikationsbeschreibung-Binäreingang](https://github.com/OpenKNX/OFM-BinaryInput/blob/v1/doc/Applikationsbeschreibung-Binaereingang.md) beschrieben.



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

## **Unterstützte Hardware**

Die Software für dieses Release wurde auf folgender Hardware getestet und läuft damit "out-of-the-box":

* [LED-Dimmer, Constant Voltage, Unterputz, 6-fach](https://www.ab-smarthouse.com/produkt/openknx-led-dimmer-constant-voltage-unterputz-6-fach/)
* [LED-Dimmer, Constant Voltage, REG 6 TE, 12-fach](https://www.ab-smarthouse.com/produkt/openknx-led-dimmer-constant-voltage-reg-6-te-12-fach/)
* [LED-Dimmer, Constant Current, Unterputz, 8-fach](https://www.ab-smarthouse.com/produkt/led-dimmer-constant-current-unterputz-8-fach/)
* [LED-Dimmer, Constant Current, Unterputz, 16-fach](https://www.ab-smarthouse.com/produkt/led-dimmer-constant-current-unterputz-16-fach/)
* [LED-Dimmer, Constant Current, REG 6 TE, 12-fach](https://www.ab-smarthouse.com/produkt/openknx-led-dimmer-constant-current-reg-6-te-12-fach/)

Andere Hardware kann genutzt werden, jedoch muss das Projekt dann neu kompiliert und gegebenenfalls angepasst werden. Alle notwendigen Teile für ein Aufsetzen der Build-Umgebung inklusive aller notwendigen Projekte finden sich im [OpenKNX-Projekt](https://github.com/OpenKNX).

Interessierte sollten auch die Beiträge im [OpenKNX-Forum](https://knx-user-forum.de/forum/projektforen/openknx) studieren.
