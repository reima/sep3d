Anmerkungen
===========
zu 5.1:
a) Bei den Spotlights wurden CutOffAngle und Exponent für eine Lichtquelle je-
   weils in ein float2 gepackt, um das Problem mit SetFloatArray (siehe
   Discussion Board) zu umgehen. Den Hinweis auf SetRawValue haben wir erst zu
   spät entdeckt und es dann einfach dabei belassen.
   Die Anzahl an Lichtquellen, die angelegt werden können, werden vom Haupt-
   programm nicht limitiert. Der Shader unterstützt allerdings nur 8 Licht-
   quellen pro Typ. Werden im Hauptprogramm darüberhinaus Lichtquellen erzeugt,
   so ist das Verhalten undefiniert.
b) Unsere Szene verwendet zwei Punkt-, eine gerichtete und vier Spotlight-
   Lichtquellen, die während der Laufzeit nicht verändert werden können. Da das
   ziemlich viel Licht auf kleinem Raum ist, empfiehlt es sich, einzelne davon
   auszukommentieren ;-)

zu 5.2:
Statt Passes haben wir Techniques verwendet, welche nicht über Tastendruck,
sondern wieder durch die Technique-Combobox angewählt werden können.
Das Lichtmodell aus Folie 17 enthält unserer Meinung nach zwei Unstimmigkeiten:
i) Die Farbe des ambienten Anteils hängt nur von der ambienten Lichtfarbe ab,
   ohne Beleuchtung erscheint das Modell also einfarbig.
ii) Der diffuse Anteil hängt nicht von der Farbe der Lichtquelle ab. Ein grünes
    Objekt erscheint also auch unter rotem Licht als grün.
Deshalb haben wir das Modell insofern abgeändert, dass der ambiente Anteil
zusätzlich mit der Geländefarbe (komponentenweise) multipliziert wird und der
diffuse Anteil (komponentenweise) mit der Lichtfarbe.
Die Farbe des ambienten Lichts ist fest auf weiß eingestellt.
Die Attenuation-Faktoren sind in der Effekt-Datei über die Konstante
vAttenuation festgelegt.
