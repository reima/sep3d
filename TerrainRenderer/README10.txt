Anmerkungen
===========

zu 10.1 a):
Die virtuelle Funktion, die die Wahrscheinlichkeit f�r das Auftreten einer 
bestimmten Vegetation berechnet, gibt diese Wahrscheinlichkeit nicht an den 
Aufrufer zur�ck, sondern entscheidet direkt, ob eine Pflanze an der 
entsprechenden Stelle gesetzt werden soll. Bei positivem Ausgang ist die Klasse 
selbst daf�r verantwortlich, die entsprechenden Daten zwischenzuspeichern (bei 
Gras wird ein struct SEED im Vektor seeds_ gespeichert).
Anstatt einer maximalen Anzahl von Billboards pro Tile gibt es eine maximale 
Gesamtanzahl f�r das komplette Terrain (momentan �ber die Konstante NUM_SEEDS in 
Terrain.cpp festgelegt).

zu 10.1 b):
Die zu rendernde Vegetation wird nicht erst in einem STL-Vektor gesammelt, 
sondern direkt beim Abstieg gerendert. Die Reihenfolge des Renderings ist dabei: 
Terrain, B�ume, Environment, Vegetation (da sonst die Vegetation durch das 
Environment verdeckt werden k�nnte).
Der Schattentest wird bei der Vegetation nicht durchgef�hrt, da sich dieser zu 
sehr negativ auf die Performance auswirkt.

zu 10.1 c):
Unsere Grasb�schel variieren leicht in ihrer Gr��e (zuf�llig vom Programm 
gesetzt) und in ihrer Textur (Texturatlas, Auswahl im Shader anhand Primitive 
ID).

zu 10.2 b):
F�r das Culling im GS in x- und y-Richtung haben wir empirisch ermittelte Werte 
verwendet.