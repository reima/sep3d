Anmerkungen
===========
zu 6.1
b) Die Screenshots liegen im Verzeichnis ./Screenshots und tragen hoffentlich
   alle selbsterkl�rende Namen.
   Unsere Kreativit�t haben wir derart ausgelebt, dass wir statt einer
   einzelnen 2D-Textur eine Volumentextur verwendet haben, wobei jedes Level
   einer anderen Gel�ndeart entspricht (Strand, 2x Wiese, 2x Gebirge, Schnee).
   Das f�r die Texturierung verwendete Level h�ngt dabei ausschlie�lich von der
   Gel�ndeh�he ab. F�r das Wasser wurde wieder Normalmapping verwendet. Die
   Materialparameter von Wasser und Gel�nde sind unterschiedlich und werden
   erst im Shader gesetzt.

zu 6.2
a) Wir haben die spiegelnde Beleuchtung so implementiert, dass zus�tzlich zu
   den spekularen Anteilen der Lichtquellen noch die Spiegelung der Umgebung
   auf den Farbwert addiert wird. Dies geschieht allerdings nur auf der
   Wasseroberfl�che. Da die Spiegelung mit Normalmapping sehr "diffus" wird,
   kann man dieses auch �ber die entsprechende Checkbox ausschalten.
b) F�r das Environment dient die neue Klasse gleichen Namens, die sich um
   das Setzen der entsprechenden Shadervariablen k�mmert und das Screen Aligned
   Quad zeichnet.

