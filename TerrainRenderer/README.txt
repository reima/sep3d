Anmerkungen
===========

zu 8.1 b) + c):
Der Hauptteil der Arbeit (Berechnung der Geländehöhe unter der Kamera) wird in 
Tile::GetHeightAt erledigt. Es wird zuerst rekursiv das Tile auf der höchsten 
(feinsten) LOD-Stufe ermittelt, über dem die Kamera sich befindet. Befindet sich 
die Kamera außerhalb des Terrains (was wir nicht verbieten/verhindern), wird das 
Tile am Rand gewählt, in das die Kamera durch Verschieben entlang der x- oder 
z-Achse gebracht werden kann. Dies ergibt insofern Sinn, als dass in diesem Fall 
die Höhe am Rand des Terrains für die Kollisionserkennung verwendet wird. Dies 
garantiert, dass im "Schwebe-Modus" keine harten Sprünge auftreten, wenn man 
sich aus dem Terrain heraus bewegt. Eine andere Möglichkeit wäre, die Höhe 
außerhalb des Terrains als 0 zu definieren (wodurch dann aber diese harten 
Sprüngen auftreten können). Durch Umkommentieren können beide Möglichkeiten 
ausprobiert werden (Tile.cpp, Z.299-303).
Für die Interpolation haben wir auch zwei Alternativen implementiert:

 * Bilineare Interpolation der vier Stützpunkte, welche zwar nicht immer die 
   tatsächliche Höhe ermittelt, aber bei genügend hoher Terrain-Auflösung 
   akzeptable Werte liefert.
 * Die korrekte Variante: lineare Interpolation über die Dreiecke des 
   Terrain-Meshes.

Ein Umschalten zur Laufzeit ist auch hier wieder nicht vorgesehen. Der 
experimentierfreudige Betreuer möge deshalb wieder Umkommentieren (Tile.cpp, 
Z.322-352).
Um zwischen Flug und Schwebe-Modus zu wechseln, betätigt man die Checkbox "Fly 
mode".

zu 8.2:
Zum Zwecke der nur einmaligen Speicherung des "Einheits-Tiles" und ähnlichem 
wurde die neue Klasse "Terrain" aus der Taufe gehoben. Sie agiert mehr oder 
weniger als Proxy-Objekt zwischen der Tile-Klasse und dem Rest der Applikation, 
das zudem alle Informationen, die alle Tiles eines zusammengehörigen Terrains 
gemein haben, speichert.
Zusätzlich zu den Höhendaten haben wir auch noch die Per-Vertex-Normalen in der 
Textur gespeichert (rgb = Normale, a = Höhe).

zu 8.3:
Die LOD-Färbung kann über die Technique-Combobox ausgewählt werden. Die 
Farbkodierung lautet (0 gröbste, 5 feinste Stufe):

 Stufe | Farbe
 ------+-------
   0   | rot
   1   | grün
   2   | blau
   3   | gelb
   4   | cyan
   5   | magenta

Der für die LOD-Wahl zu verwendende maximale Screen Space Error kann über den 
Schieberegler "Screen error" gesetzt werden.

zum Hinweis:
Die Skalierung des Höhenfeldes in xz-Richtung kann nun im "New Terrain"-Dialog 
angepasst werden ("Scale"). Standardwert ist 10 (Seitenlänge des Terrains 10 
Längeneinheiten). Der Wertebereich der berechneten Höhen passt sich automatisch 
dieser Skalierung an.
Zusätzlich kann nun auch die Bewegungsgeschwindigkeit der Kamera über den 
Schieberegler "Camera Speed" angepasst werden.
