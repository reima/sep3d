Besonderheiten:

* Das Projekt geht davon aus, dass die Header- und Bibliotheksverzeichnisse des
  DirectX-SDKs bereits global in Visual Studio eingebunden sind.
  
* Die ModelViewCamera wurde durch eine FirstPersonCamera ausgetauscht.
  
* Wir haben die GUI um ein paar zusätzliche Elemente erweitert:
  - Checkbox zum An- und Ausschalten der Wireframe-Anzeige
  - Slider zur Wahl der anzuzeigenden LOD-Stufe
  - Dialog zum Einstellen der Terrain-Parameter und Möglichkeit, ein neues
    Terrain generieren zu lassen. Die #defines im Quellcode dienen als vorein-
    gestellte Standardparameter.
    
* Der Vertex Shader wurde etwas aufgebohrt:
  - Die Berechnung der Vertexfarbe wurde praktisch 1:1 von der letzten Aufgabe
    übernommen und in HLSL umgesetzt.
  - Der Shader kümmert sich auch um die Generierung einer animierten Wasser-
    oberfläche (unter Verwendung des Effektparameters g_fTime).