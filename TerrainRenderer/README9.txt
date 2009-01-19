Anmerkungen
===========

zu 9.1 b):
Die B�ume werden abh�ngig von der H�he und der Steilheit des Terrains platziert.
Dazu wird wiederholt eine zuf�llige Position im Terrain ausgew�hlt und diese auf
die passenden Gegebenheiten hin �berpr�ft. Treffen alle Bedingungen zu, wird ein
Baum an diese Stelle gesetzt, andernfalls nicht. Insgesamt werden 250 "Versuche"
unternommen, sodass am Ende zwischen 0 und 250 B�ume tats�chlich im Terrain
landen. Die Anzahl der generierten B�ume werden im Hilfetext angezeigt (Taste H)

Sonstiges:
Wir haben in diese Abgabe eine experimentelle Implementierung von Trapezoidal
Shadow Mapping nach Martin und Tan[1] eingebaut. Man kann sie durch Dr�cken von
T ein- und ausschalten. Sie hat allerdings in speziellen Situationen noch
ziemliche Probleme:
* Der "Duelling Frustra"-Fall wird nicht gesondert behandelt und f�hrt zu
  drastischer Verschlechterung der Aufl�sung im Nahbereich.
* Die Berechnung der konvexen H�lle schl�gt in manchen F�llen fehl (vermutlich
  auf Grund numerischer Instabilit�t der Implementierung).
* Der Schnitt des View Frustrums mit dem Light Space wird nicht berechnet,
  stattdessen wird einfach das View Frustrum verwendet.

[1]: http://www.comp.nus.edu.sg/~tants/tsm.html