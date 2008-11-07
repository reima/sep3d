Besonderheiten:

* Die �bereinstimmung der Randknoten von benachbarten Tiles in einem Detail-
  Level wurde so gel�st, dass sich jedes Tile die Randknoten seines n�rdlichen
  und seines westlichen Nachbarn kopiert. Die geschieht in der Methode
  Tile::FixEdges. Dieses Verfahren ist zwar relativ einfach zu implementieren,
  allerdings nicht ganz korrekt in dem Sinne, dass dadurch "unnat�rliche"
  �berg�nge entstehen k�nnen. Die �stlichen und s�dlichen Randknoten jedes
  Tiles werden n�mlich stets nur anhand der Informationen im aktuellen Tile
  berechnet. F�r den Diamond-Step m�sste aber eigentlich korrekterweise auf
  Knoten im angrenzenden Nachbarn zur�ckgegriffen werden. Unterscheiden sich
  deren H�henwerte massiv von den beteiligten H�henwerten im aktuellen Tile,
  so kommt es dadurch m�glicherweise zu gro�en H�henunterschieden, die sich im
  H�henfeld als Rauschen �u�ern.
  Wir haben uns dennoch f�r diese Variante entschieden, da f�r gen�gend gro�e
  Werte von n (> 6) dieser Effekt vernachl�ssigbar wird.
  
* Nicht gefordert, aber f�r Debugging-Zwecke als n�tzlich erwiesen hat sich die
  Methode Tile::SaveObjs. Sie speichert f�r jedes Tile des Quadtrees die
  Vertices und die durch die Triangulierung entstandenen Dreiecke in eine
  OBJ-Datei (eine pro Tile). Diese lassen sich mit einem passenden Viewer (z.B.
  MeshLab) oder in fast jedem beliebigen 3D-Programm �ffnen und inspizieren.
