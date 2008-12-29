Anmerkungen
===========

zu 9.1 b):
Die Bäume werden abhängig von der Höhe und der Steilheit des Terrains platziert.
Dazu wird wiederholt eine zufällige Position im Terrain ausgewählt und diese auf
die passenden Gegebenheiten hin überprüft. Treffen alle Bedingungen zu, wird ein
Baum an diese Stelle gesetzt, andernfalls nicht. Insgesamt werden 250 "Versuche"
unternommen, sodass am Ende zwischen 0 und 250 Bäume tatsächlich im Terrain
landen. Die Anzahl der generierten Bäume werden im Hilfetext angezeigt (Taste H)

Sonstiges:
Wir haben in diese Abgabe eine experimentelle Implementierung von Trapezoidal
Shadow Mapping nach Martin und Tan[1] eingebaut. Man kann sie durch Drücken von
T ein- und ausschalten. Sie hat allerdings in speziellen Situationen noch
ziemliche Probleme:
* Der "Duelling Frustra"-Fall wird nicht gesondert behandelt und führt zu
  drastischer Verschlechterung der Auflösung im Nahbereich.
* Die Berechnung der konvexen Hülle schlägt in manchen Fällen fehl (vermutlich
  auf Grund numerischer Instabilität der Implementierung).
* Der Schnitt des View Frustrums mit dem Light Space wird nicht berechnet,
  stattdessen wird einfach das View Frustrum verwendet.

[1]: http://www.comp.nus.edu.sg/~tants/tsm.html