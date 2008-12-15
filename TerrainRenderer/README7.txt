Anmerkungen
===========
* Unsere Szene enth�lt standardm��ig eine direktionale, schattenwerfende Licht-
  quelle (Farbe orange), eine schattenwerfende Punktlichtquelle (Farbe rot),
  eine normale Punktlichtquelle (Farbe gr�n) und ein Spotlight (Farbe gelb).
  Hinzuf�gen und Entfernen von Lichtquellen klappt nur durch Anpassen des
  Quellcodes (TerrainRenderer.cpp ab Zeile 425) und neu kompilieren. Schatten-
  werfende Lichtquellen k�nnen zwischen 0 und 1 mal pro Typ (direktional, Punkt)
  vorkommen. F�gt man der Szene dar�berhinaus schattenwerfende Lichtquellen
  hinzu, so ist das Verhalten undefiniert (insbesondere wird keine Warnung
  ausgegeben).
* Neues Killer-Feature: Der Pausen-Modus (F6).
* W�hrend der Laufzeit kann die Schattenberechnung �ber die Parameter
  - Shadow Map Aufl�sung (SM Res.)
  - Shadow Map Genauigkeit (High Prec. SM = 32-Bit-Textur, sonst 16-Bit)
  - Depth Bias (Z epsilon)
  - Filtering (3x3 PCF oder Point)
  anpassen. Die Einstellungen wirken sich auf s�mtliche schattenwerfende Licht-
  quellen in der Szene gleichzeitig aus.
* Die Shader sind ziemlich unoptimiert und enthalten noch einige Redundanzen.
* Die im Debug-Modus pro Frame ausgegebenen Warnungen
  
  D3D10: WARNING: ID3D10Device::OMSetRenderTargets: Resource being set to OM DepthStencil is still bound on input! [ STATE_SETTING WARNING #9: DEVICE_OMSETRENDERTARGETS_HAZARD ]
  D3D10: WARNING: ID3D10Device::OMSetRenderTargets: Forcing PS shader resource slot 3 to NULL. [ STATE_SETTING WARNING #7: DEVICE_PSSETSHADERRESOURCES_HAZARD ]
  D3D10: WARNING: ID3D10Device::OMSetRenderTargets: Resource being set to OM DepthStencil is still bound on input! [ STATE_SETTING WARNING #9: DEVICE_OMSETRENDERTARGETS_HAZARD ]
  D3D10: WARNING: ID3D10Device::OMSetRenderTargets: Forcing PS shader resource slot 4 to NULL. [ STATE_SETTING WARNING #7: DEVICE_PSSETSHADERRESOURCES_HAZARD ]
  
  sind uns bekannt. Wie man sie wegbekommt, ohne einen Handstand zu machen,
  wissen wir hingegen nicht.
* Der Quellcode ist nicht gerade eine Software-Engineering-Meisterleistung.
  Aber hey, es l�uft (meistens sogar ohne abzust�rzen) ;-)