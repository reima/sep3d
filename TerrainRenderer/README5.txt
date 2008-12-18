Anmerkungen
===========
zu 5.1:
a) Bei den Spotlights wurden CutOffAngle und Exponent f�r eine Lichtquelle je-
   weils in ein float2 gepackt, um das Problem mit SetFloatArray (siehe
   Discussion Board) zu umgehen. Den Hinweis auf SetRawValue haben wir erst zu
   sp�t entdeckt und es dann einfach dabei belassen.
   Die Anzahl an Lichtquellen, die angelegt werden k�nnen, werden vom Haupt-
   programm nicht limitiert. Der Shader unterst�tzt allerdings nur 8 Licht-
   quellen pro Typ. Werden im Hauptprogramm dar�berhinaus Lichtquellen erzeugt,
   so ist das Verhalten undefiniert.
b) Unsere Szene verwendet zwei Punkt-, eine gerichtete und vier Spotlight-
   Lichtquellen, die w�hrend der Laufzeit nicht ver�ndert werden k�nnen. Da das
   ziemlich viel Licht auf kleinem Raum ist, empfiehlt es sich, einzelne davon
   auszukommentieren ;-)

zu 5.2:
Statt Passes haben wir Techniques verwendet, welche nicht �ber Tastendruck,
sondern wieder durch die Technique-Combobox angew�hlt werden k�nnen.
Das Lichtmodell aus Folie 17 enth�lt unserer Meinung nach zwei Unstimmigkeiten:
i) Die Farbe des ambienten Anteils h�ngt nur von der ambienten Lichtfarbe ab,
   ohne Beleuchtung erscheint das Modell also einfarbig.
ii) Der diffuse Anteil h�ngt nicht von der Farbe der Lichtquelle ab. Ein gr�nes
    Objekt erscheint also auch unter rotem Licht als gr�n.
Deshalb haben wir das Modell insofern abge�ndert, dass der ambiente Anteil
zus�tzlich mit der Gel�ndefarbe (komponentenweise) multipliziert wird und der
diffuse Anteil (komponentenweise) mit der Lichtfarbe.
Die Farbe des ambienten Lichts ist fest auf wei� eingestellt.
Die Attenuation-Faktoren sind in der Effekt-Datei �ber die Konstante
vAttenuation festgelegt.
