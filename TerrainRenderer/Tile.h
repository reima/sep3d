#pragma once
#include <string>

/**
 * H�henfeld-Tile
 */
class Tile {
 public:
  /**
   * Konstruktor.
   * @param n Detaillevel, legt die Gr��e des Tiles (2^n - 1) fest
   * @param roughness Rauheits-Faktor (je h�her desto gr��er die
   *                  H�henunterschiede)
   * @param num_lod Anzahl zus�tzlicher LOD-Ebenen
   */
  Tile(int n, float roughness, int num_lod);
  ~Tile(void);

  /** 
   * Gibt die Aufl�sung des Tiles zur�ck.
   */
  int GetResolution() const { return size_ * size_; }

  /**
   * Gibt die Detailstufe (LOD) des Tiles zur�ck.
   */
  int GetLOD() const { return lod_; }

  /**
   * Ermittelt die minimale H�he im Tile und gibt sie zur�ck.
   */
  float GetMinHeight() const;

  /**
   * Ermittelt die maximale H�he im Tile und gibt sie zur�ck.
   */
  float GetMaxHeight() const;

  /**
   * Trianguliert streifenweise.
   */
  void TriangulateLines(void);

  /**
   * Trianguliert mit Z-Order.
   */
  void TriangulateZOrder(void);

  /**
   * Speichert Terrain-Mesh f�r jedes Tile im OBJ-Dateiformat.
   */
  void SaveObjs(const std::wstring &filename) const;

 private:
  /**
   * Richtungstyp, der einen Quadranten eines Tiles spezifiziert
   */
  enum Direction { NW = 0, NE, SW, SE };
  /**
   * 3-dimensionaler Vektor, wird f�r Vertices verwendet.
   * @see Tile::vertices_
   */
  struct Vector { float x, y, z; };

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
  Tile(Tile *parent, Direction direction, float roughness, Tile *north,
       Tile *west);

  /**
   * Rekursive Implementierung von SaveObjs
   */
  void SaveObjs0(const std::wstring &basename,
                 const std::wstring &extension) const;

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

  /**
   * Reserviert Speicher f�r den Index Buffer.
   */
  void InitIndexBuffer(void);

  /**
   * Rekursive Implementierung der Z-Order-Triangulierung.
   */
  void TriangulateZOrder0(int x1, int y1, int x2, int y2, int &i);

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
   * Feld der Vertices dieses Tiles
   */
  Vector *vertices_;
  /**
   * Indizes f�r die Triangulierung des H�henfeldes
   * @see Tile::TriangulateLines
   * @see Tile::TriangulateZOrder
   */
  unsigned int *index_buffer_;
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
};
