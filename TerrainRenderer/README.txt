Techniques
==========
Wir haben insgesamt 5 unterschiedliche Techniques implementiert, die über eine
entsprechende Combobox ausgewählt werden können:
* "Vertex Coloring": höhenabhängige Farbkodierung im Vertexshader (siehe 4.2 b))
* "Vertex Col. + Diffuse": wie oben, nur mit Beleuchtungssimulation gemäß dem
  Lambertschen Gesetz (ideal diffuse Fläche). Als Lichtquelle dient dabei die
  Kameraposition.
* "Pixel Coloring": höhenabhängige Farbkodierung im Pixelshader (siehe 4.2 c))
* "Normal Coloring": Einfärbung gemäß der in 4.1 berechneten Per-Vertex-
  Normalen.
* "Special FX": Hier habe ich (Matthias) mich außerhalb der Aufgabenstellung
  ausgetobt und einen halbwegs realistischen Wasser-Shader entworfen. Haupt-
  features sind dynamisch im Vertex-Shader berechnete Wellen, eine über Normal
  Mapping detailliert gestaltete Wasseroberfläche, welche mittels Phong-Shading
  dargestellt wird. Außerdem wird die Transparenz des Wassers blickwinkelab-
  hängig durch einen einfachen approximativen Fresnel-Term bestimmt. Fast alle
  Parameter dieses Shaders lassen sich interaktiv in der Anwendung anpassen.
  Dazu dienen die in drei Kategorien (Light, Waves, Water) unterteilten Schiebe-
  regler, die unter "SFX Settings" zusammengefasst sind. Auf die anderen
  Techniques haben diese Regler keinen Einfluss. Meiner Meinung nach ästhetisch
  ansprechende Standardeinstellungen sind beim Start vorgegeben.
  
Anmerkungen
===========
zu 4.2:
a) Die von minimaler/maximaler Höhe des aktuellen Terrains abhängige Farbgebung
   kann durch die Checkbox "Dynamic Min/Max" an- und ausgeschaltet werden.
b) Der Screenshot ist <./Screenshots/vertex-coloring.png>. Die Blickrichtung
   ist in etwa orthogonal zur y-Achse.
c) Der Screenshot ist <./Screenshots/pixel-coloring.png>. Der Blickwinkel ent-
   spricht genau dem aus Teilaufgabe b). Die teils falsche Farbgebung bei Be-
   stimmung der Farben im Vertex-Shader entsteht dadurch, dass sich ein Drei-
   eck über mehrere Höhenfarbstufen erstrecken kann. Der Vertex-Shader bestimmt
   nur die korrekte Farben für die Eckpunkte, welche dann bei der Rasterisierung
   (perspektivisch korrekt) linear auf die Fragmente interpoliert werden. Dabei
   werden die möglicherweise dazwischen liegenden Höhenfarbstufen nicht beachtet.
   Ein Höhenwert wird dagegen korrekt interpoliert, sodass bei der Färbung im
   Pixel-Shader eine einheitliche Farbgebung entsteht.
   
zu 4.3:
a) Die zu rendernde LOD-Stufe kann nun (zusätzlich zum Schieberegler) auch über
   die Tasten "+" und "-" angepasst werden.
b) Bei der Textausgabe haben wir uns auf die zur Generierung des aktuellen
   Terrains verwendeten Parameter beschränkt. Die aktuellen Werte aller anderen
   Parameter sind ja schon aus den GUI-Elementen ablesbar.
 