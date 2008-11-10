Techniques
==========
Wir haben insgesamt 5 unterschiedliche Techniques implementiert, die �ber eine
entsprechende Combobox ausgew�hlt werden k�nnen:
* "Vertex Coloring": h�henabh�ngige Farbkodierung im Vertexshader (siehe 4.2 b))
* "Vertex Col. + Diffuse": wie oben, nur mit Beleuchtungssimulation gem�� dem
  Lambertschen Gesetz (ideal diffuse Fl�che). Als Lichtquelle dient dabei die
  Kameraposition.
* "Pixel Coloring": h�henabh�ngige Farbkodierung im Pixelshader (siehe 4.2 c))
* "Normal Coloring": Einf�rbung gem�� der in 4.1 berechneten Per-Vertex-
  Normalen.
* "Special FX": Hier habe ich (Matthias) mich au�erhalb der Aufgabenstellung
  ausgetobt und einen halbwegs realistischen Wasser-Shader entworfen. Haupt-
  features sind dynamisch im Vertex-Shader berechnete Wellen, eine �ber Normal
  Mapping detailliert gestaltete Wasseroberfl�che, welche mittels Phong-Shading
  dargestellt wird. Au�erdem wird die Transparenz des Wassers blickwinkelab-
  h�ngig durch einen einfachen approximativen Fresnel-Term bestimmt. Fast alle
  Parameter dieses Shaders lassen sich interaktiv in der Anwendung anpassen.
  Dazu dienen die in drei Kategorien (Light, Waves, Water) unterteilten Schiebe-
  regler, die unter "SFX Settings" zusammengefasst sind. Auf die anderen
  Techniques haben diese Regler keinen Einfluss. Meiner Meinung nach �sthetisch
  ansprechende Standardeinstellungen sind beim Start vorgegeben.
  
Anmerkungen
===========
zu 4.2:
a) Die von minimaler/maximaler H�he des aktuellen Terrains abh�ngige Farbgebung
   kann durch die Checkbox "Dynamic Min/Max" an- und ausgeschaltet werden.
b) Der Screenshot ist <./Screenshots/vertex-coloring.png>. Die Blickrichtung
   ist in etwa orthogonal zur y-Achse.
c) Der Screenshot ist <./Screenshots/pixel-coloring.png>. Der Blickwinkel ent-
   spricht genau dem aus Teilaufgabe b). Die teils falsche Farbgebung bei Be-
   stimmung der Farben im Vertex-Shader entsteht dadurch, dass sich ein Drei-
   eck �ber mehrere H�henfarbstufen erstrecken kann. Der Vertex-Shader bestimmt
   nur die korrekte Farben f�r die Eckpunkte, welche dann bei der Rasterisierung
   (perspektivisch korrekt) linear auf die Fragmente interpoliert werden. Dabei
   werden die m�glicherweise dazwischen liegenden H�henfarbstufen nicht beachtet.
   Ein H�henwert wird dagegen korrekt interpoliert, sodass bei der F�rbung im
   Pixel-Shader eine einheitliche Farbgebung entsteht.
   
zu 4.3:
a) Die zu rendernde LOD-Stufe kann nun (zus�tzlich zum Schieberegler) auch �ber
   die Tasten "+" und "-" angepasst werden.
b) Bei der Textausgabe haben wir uns auf die zur Generierung des aktuellen
   Terrains verwendeten Parameter beschr�nkt. Die aktuellen Werte aller anderen
   Parameter sind ja schon aus den GUI-Elementen ablesbar.
 