### Kanalzuordnung

Hier erfolgt die Zuordnung von Dimmkanälen zu den verfügbaren Hardwarekanälen (A, B, C, ...).

Dazu wird zunächst der Typ ausgewählt. Aktuell werden unterstützt:

* EK - Einzelkanal
* TW - Tunable White
* RGB - Rot/Grün/Blau

Danach wird die Nummer des Dimmkanals vom gewählten Typ zugeordnet, die für den Hardwarekanal verwendet werden soll, wobei eine "0" deaktiviert meint.

Beispiel: Hardwarekanäle "A" und "B" sollen zusammen Tunable White LEDs dimmen, dann wird beiden Kanälen der Typ "TW - Tunable White" zugeordnet und der Dimmkanal bei beiden auf "1" gesetzt. Möchte man nun auch mit den Hardwarekanälen "C" und "D" andere Tunable White LEDs dimmen, so wählt man denselben Typ aus, jedoch weißt Dimmkanal "2" zu.

In der letzten Spalte wird noch die Funktion innerhalb des Dimmkanals zugeordnet. Bei Tunable White wäre das "Warmweiß" und "Kaltweiß", bei RGB-Kanäle entsprechend die drei Farben.

