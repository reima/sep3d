Anmerkungen
===========
zu 6.1
b) Die Screenshots liegen im Verzeichnis ./Screenshots und tragen hoffentlich
   alle selbsterklärende Namen.
   Unsere Kreativität haben wir derart ausgelebt, dass wir statt einer
   einzelnen 2D-Textur eine Volumentextur verwendet haben, wobei jedes Level
   einer anderen Geländeart entspricht (Strand, 2x Wiese, 2x Gebirge, Schnee).
   Das für die Texturierung verwendete Level hängt dabei ausschließlich von der
   Geländehöhe ab. Für das Wasser wurde wieder Normalmapping verwendet. Die
   Materialparameter von Wasser und Gelände sind unterschiedlich und werden
   erst im Shader gesetzt.

zu 6.2
a) Wir haben die spiegelnde Beleuchtung so implementiert, dass zusätzlich zu
   den spekularen Anteilen der Lichtquellen noch die Spiegelung der Umgebung
   auf den Farbwert addiert wird. Dies geschieht allerdings nur auf der
   Wasseroberfläche. Da die Spiegelung mit Normalmapping sehr "diffus" wird,
   kann man dieses auch über die entsprechende Checkbox ausschalten.
b) Für das Environment dient die neue Klasse gleichen Namens, die sich um
   das Setzen der entsprechenden Shadervariablen kümmert und das Screen Aligned
   Quad zeichnet.

