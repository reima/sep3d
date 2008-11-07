Besonderheiten:

* Die Übereinstimmung der Randknoten von benachbarten Tiles in einem Detail-
  Level wurde so gelöst, dass sich jedes Tile die Randknoten seines nördlichen
  und seines westlichen Nachbarn kopiert. Die geschieht in der Methode
  Tile::FixEdges. Dieses Verfahren ist zwar relativ einfach zu implementieren,
  allerdings nicht ganz korrekt in dem Sinne, dass dadurch "unnatürliche"
  Übergänge entstehen können. Die östlichen und südlichen Randknoten jedes
  Tiles werden nämlich stets nur anhand der Informationen im aktuellen Tile
  berechnet. Für den Diamond-Step müsste aber eigentlich korrekterweise auf
  Knoten im angrenzenden Nachbarn zurückgegriffen werden. Unterscheiden sich
  deren Höhenwerte massiv von den beteiligten Höhenwerten im aktuellen Tile,
  so kommt es dadurch möglicherweise zu großen Höhenunterschieden, die sich im
  Höhenfeld als Rauschen äußern.
  Wir haben uns dennoch für diese Variante entschieden, da für genügend große
  Werte von n (> 6) dieser Effekt vernachlässigbar wird.
  
* Nicht gefordert, aber für Debugging-Zwecke als nützlich erwiesen hat sich die
  Methode Tile::SaveObjs. Sie speichert für jedes Tile des Quadtrees die
  Vertices und die durch die Triangulierung entstandenen Dreiecke in eine
  OBJ-Datei (eine pro Tile). Diese lassen sich mit einem passenden Viewer (z.B.
  MeshLab) oder in fast jedem beliebigen 3D-Programm öffnen und inspizieren.
