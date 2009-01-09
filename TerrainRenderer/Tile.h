#pragma once
#include <string>
#include "DXUT.h"
#include "DXUTCamera.h"

class LODSelector;
class Terrain;
class Vegetation;

/**
 * H�henfeld-Tile
 */
class Tile {
 friend class Terrain;

 public:
  /**
   * Konstruktor.
   * @param terrain Das Terrain, zu dem das Tile geh�rt
   * @param n Detaillevel, legt die Gr��e des Tiles (2^n - 1) fest
   * @param roughness Rauheits-Faktor (je h�her desto gr��er die
   *                  H�henunterschiede)
   * @param num_lod Anzahl zus�tzlicher LOD-Ebenen
   */
  Tile(Terrain *terrain, int n, float roughness, int num_lod, float scale,
       bool water);
  ~Tile(void);

  /**
   * Gibt die Aufl�sung des Tiles zur�ck.
   */
  int GetResolution(void) const { return size_ * size_; }

  /**
   * Gibt die Detailstufe (LOD) des Tiles zur�ck.
   */
  int GetLOD(void) const { return lod_; }

  /**
   * Ermittelt die minimale H�he im Tile und gibt sie zur�ck.
   */
  float GetMinHeight(void) const;

  /**
   * Ermittelt die maximale H�he im Tile und gibt sie zur�ck.
   */
  float GetMaxHeight(void) const;

  float GetHeightAt(const D3DXVECTOR3 &pos) const;
  D3DXVECTOR3 GetNormalAt(const D3DXVECTOR3 &pos) const;

  /**
   * Erzeugt Vertex-, Normalen- und Index-Buffer l�dt das Dreieckgitter des
   * Tiles in diese hoch. Wird ggf. rekursiv f�r alle Kinder des Tiles
   * aufgerufen.
   * @warning Das Tile muss zuvor trianguliert worden sein (durch Aufruf von
   *          Tile::TriangulateLines oder Tile::TriangulateZOrder).
   * @see Tile::ReleaseBuffers
   */
  HRESULT CreateBuffers(ID3D10Device *device);

  /**
   * Rendert das Tile.
   * @param lod_selector Ein LODSelector, der bestimmt, ob die LOD-Stufe des
   *                     Tiles ausreicht. Wenn nicht, werden rekursiv die
   *                     Kinder des Tiles gezeichnet.
   * @param cam_pos Die Position der Kamera, zur �bergabe an den LODSelector.
   * @warning Vor dem Aufruf m�ssen die D3D10-Buffer mit Tile::CreateBuffers
   *          erzeugt werden.
   */
  void Draw(LODSelector *lod_selector, const CBaseCamera *camera);
  void DrawVegetation(void);
  /**
   * Gibt den f�r die interne Darstellung reservierten Speicher frei (auch
   * rekursiv f�r alle Kind-Tiles).
   * @warning Die Methoden Tile::GetMinHeight, Tile::GetMaxHeight,
   *          Tile::SaveObjs, Tile::CreateBuffers werden danach _nicht_ mehr
   *          funktionieren. Tile::Draw wird unabh�ngig vom Aufruf dieser
   *          Methode weiterarbeiten, sofern vorher Tile::CreateBuffers
   *          aufgerufen wurde.
   */
 void FreeMemory(void);

  /**
   * Berechnet die Normalen des Terrains.
   */
  void CalculateNormals(unsigned int *indices);

  void GetBoundingBox(D3DXVECTOR3 *out, D3DXVECTOR3 *mid) const;

  float GetWorldError(void) const;

 private:
  /**
   * Richtungstyp, der einen Quadranten eines Tiles spezifiziert
   */
  enum Direction { NW = 0, NE, SW, SE };

  // Kopierkonstruktor und Zuweisungsoperator verbieten.
  Tile(const Tile &t);
  void operator=(const Tile &t);

  /**
   * Konstruktor f�r Kind-Tiles.
   * @param parent Eltern-Tile
   * @param direction Quadrant des Eltern-Tiles, in dem dieses Tile liegt
   * @param roughness Rauheits-Faktor f�r die weitere Verfeinerung
   * @param north Zeiger auf n�rdlichen Nachbarn (oder NULL)
   * @param west Zeiger auf westlichen Nachbarn (oder NULL)
   */
  Tile(Tile *parent, Direction direction, float roughness,
       Tile *north, Tile *west);

  /**
   * Initialisierungsfunktion f�r das Wurzel-Tile. Setzt die anf�nglichen
   * Zufallswerte und berechnet x- und z-Koordinaten aller Vertices vor.
   */
  void Init(float roughness);
  /**
   * Initialisierungsfunktion f�r ein Kind-Tile. �bernimmt die Werte aus dem
   * entsprechenden Quadranten des Eltern-Tiles und berechnet x- und z-
   * Koordinaten aller Vertices vor.
   */
  void InitFromParent(void);
  /**
   * F�hrt die Verfeinerung der H�heninformationen nach dem
   * Diamond-Square-Algorithmus zur Blockgr��e block_size durch.
   */
  void Refine(int block_size, float roughness);
  /**
   * Initialisiert die Kind-Tiles, falls n�tig.
   * @param roughness Rauheits-Faktor f�r die Verfeinerung der Kind-Tiles
   * @param north N�rdlicher Nachbar dieses Tiles
   * @param west Westlicher Nachbar dieses Tiles
   */
  void InitChildren(float roughness, Tile *north, Tile *west);

  /**
   * "Repariert" die �berg�nge des Tiles. �bernimmt die s�dlichste Zeile des
   * n�rdlichen Nachbarn und �stlichste Spalte des westlichen Nachbarn.
   */
  void FixEdges(Tile *north, Tile *west);

  void CreateWater(void);

  inline D3DXVECTOR3 GetVectorFromIndex(int index);
  /**
   * Rekursive Implementierung von CalculateNormals
   */
  void CalculateNormals0(Tile *north, Tile *west, unsigned int *indices);

  /**
   * Normalisiert alle Vertex-Normalen.
   */
  void NormalizeNormals(void);

  /**
   * Gibt die von CreateBuffers erzeugten Buffer wieder frei (nicht rekursiv).
   */
  void ReleaseBuffers(void);

  void CalculateHeights(void);

  void PlaceVegetation(const D3DXVECTOR3 &position);

  void GrowVegetation(void);

  /**
   * Terrain, zu der dieses Tile geh�rt
   */
  Terrain *terrain_;
  /**
   * LOD-Stufe des Tiles
   */
  const int lod_;
  /**
   * Gr��e des Tiles (Seitenl�nge)
   */
  const int size_;
  /**
   * Anzahl zus�tzlicher LOD-Ebenen unter diesem Tile
   */
  const int num_lod_;

  /**
   * Feld der H�hen dieses Tiles
   */
  float *heights_;

  /**
   * Feld der Per-Vertex-Normalen dieses Tiles
   */
  D3DXVECTOR3 *vertex_normals_;
  /**
   * �bergeordnetes Eltern-Tile
   */
  Tile *parent_;
  /**
   * Quadrant des Eltern-Tiles, in dem dieses Tile liegt
   */
  Direction direction_;
  /**
   * Feld der Kind-Tiles (falls vorhanden)
   */
  Tile *children_[4];

  float max_height_;
  float min_height_;
  float scale_;
  D3DXVECTOR2 translation_;

  ID3D10Texture2D *height_map_;
  ID3D10ShaderResourceView *shader_resource_view_;

  Vegetation *vegetation_;
  ID3D10Device *device_;
};

