Anmerkungen
===========

zu 8.1 b) + c):
Der Hauptteil der Arbeit (Berechnung der Gel�ndeh�he unter der Kamera) wird in 
Tile::GetHeightAt erledigt. Es wird zuerst rekursiv das Tile auf der h�chsten 
(feinsten) LOD-Stufe ermittelt, �ber dem die Kamera sich befindet. Befindet sich 
die Kamera au�erhalb des Terrains (was wir nicht verbieten/verhindern), wird das 
Tile am Rand gew�hlt, in das die Kamera durch Verschieben entlang der x- oder 
z-Achse gebracht werden kann. Dies ergibt insofern Sinn, als dass in diesem Fall 
die H�he am Rand des Terrains f�r die Kollisionserkennung verwendet wird. Dies 
garantiert, dass im "Schwebe-Modus" keine harten Spr�nge auftreten, wenn man 
sich aus dem Terrain heraus bewegt. Eine andere M�glichkeit w�re, die H�he 
au�erhalb des Terrains als 0 zu definieren (wodurch dann aber diese harten 
Spr�ngen auftreten k�nnen). Durch Umkommentieren k�nnen beide M�glichkeiten 
ausprobiert werden (Tile.cpp, Z.299-303).
F�r die Interpolation haben wir auch zwei Alternativen implementiert:

 * Bilineare Interpolation der vier St�tzpunkte, welche zwar nicht immer die 
   tats�chliche H�he ermittelt, aber bei gen�gend hoher Terrain-Aufl�sung 
   akzeptable Werte liefert.
 * Die korrekte Variante: lineare Interpolation �ber die Dreiecke des 
   Terrain-Meshes.

Ein Umschalten zur Laufzeit ist auch hier wieder nicht vorgesehen. Der 
experimentierfreudige Betreuer m�ge deshalb wieder Umkommentieren (Tile.cpp, 
Z.322-352).
Um zwischen Flug und Schwebe-Modus zu wechseln, bet�tigt man die Checkbox "Fly 
mode".

zu 8.2:
Zum Zwecke der nur einmaligen Speicherung des "Einheits-Tiles" und �hnlichem 
wurde die neue Klasse "Terrain" aus der Taufe gehoben. Sie agiert mehr oder 
weniger als Proxy-Objekt zwischen der Tile-Klasse und dem Rest der Applikation, 
das zudem alle Informationen, die alle Tiles eines zusammengeh�rigen Terrains 
gemein haben, speichert.
Zus�tzlich zu den H�hendaten haben wir auch noch die Per-Vertex-Normalen in der 
Textur gespeichert (rgb = Normale, a = H�he).

zu 8.3:
Die LOD-F�rbung kann �ber die Technique-Combobox ausgew�hlt werden. Die 
Farbkodierung lautet (0 gr�bste, 5 feinste Stufe):

 Stufe | Farbe
 ------+-------
   0   | rot
   1   | gr�n
   2   | blau
   3   | gelb
   4   | cyan
   5   | magenta

Der f�r die LOD-Wahl zu verwendende maximale Screen Space Error kann �ber den 
Schieberegler "Screen error" gesetzt werden.

zum Hinweis:
Die Skalierung des H�henfeldes in xz-Richtung kann nun im "New Terrain"-Dialog 
angepasst werden ("Scale"). Standardwert ist 10 (Seitenl�nge des Terrains 10 
L�ngeneinheiten). Der Wertebereich der berechneten H�hen passt sich automatisch 
dieser Skalierung an.
Zus�tzlich kann nun auch die Bewegungsgeschwindigkeit der Kamera �ber den 
Schieberegler "Camera Speed" angepasst werden.
