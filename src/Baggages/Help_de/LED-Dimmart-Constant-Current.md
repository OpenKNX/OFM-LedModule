### Dimmart Constant Current


Hier kann die Art der Dimmung gewählt werden:

* Hybrides dimmen (analog/PWM): Hierbei wird oberhalb von 12,5 % Dimmwert analog gedimmt. Das bedeutet, dass der Constant Current entsprechend des Dimmwerts reduziert wird. Ist also beispielsweise ein Constant Current von 500 mA per Hardware-Widerstand für einen Kanal vorgegeben und der aktuell Dimmwert beträgt 50 %, so begrenzt der Dimmer den Constant Current auf lediglich 250 mA. Die Auflösung beträgt hierbei 8 Bit (= 256 Werte). Da die Reduktion des Constant Current nicht unbegrenzt funktioniert, wird unterhalb von 12,5 % Dimmwert immer PWM gedimmt.

* Reines PWM dimmen: Grundsätzlich ist die Empfehlung hybrides dimmen zu verwenden, um die Vorteile einer Konstantstromquelle ohne sichtbares "Flackern" auszunutzen. Sollte in bestimmten Situation jedoch eine Auflösung von 8 Bit nicht ausreichen, so kann der Dimmer hier auf reines PWM dimmen umgeschaltet werden.

