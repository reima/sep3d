#pragma once
#include <string>

/**
 * Höhenfeld-Tile
 */
class Tile {
 public:
  /**
   * Konstruktor.
   * @param n Detaillevel, legt die Größe des Tiles (2^n - 1) fest
   * @param roughness Rauheits-Faktor (je höher desto größer die
   *                  Höhenunterschiede)
   * @param num_lod Anzahl zusätzlicher LOD-Ebenen
   */
  Tile(int n, float roughness, int num_lod);
  ~Tile(void);

  /** 
   * Gibt die Auflösung des Tiles zurück.
   */
  int GetResolution() const { return size_ * size_; }

  /**
   * Gibt die Detailstufe (LOD) des Tiles zurück.
   */
  int GetLOD() const { return lod_; }

  /**
   * Ermittelt die minimale Höhe im Tile und gibt sie zurück.
   */
  float GetMinHeight() const;

  /**
   * Ermittelt die maximale Höhe im Tile und gibt sie zurück.
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
   * Speichert Terrain-Mesh für jedes Tile im OBJ-Dateiformat.
   */
  void SaveObjs(const std::wstring &filename) const;

 private:
  /**
   * Richtungstyp, der einen Quadranten eines Tiles spezifiziert
   */
  enum Direction { NW = 0, NE, SW, SE };
  /**
   * 3-dimensionaler Vektor, wird für Vertices verwendet.
   * @see Tile::vertices_
   */
  struct Vector { float x, y, z; };

  // Kopierkonstruktor und Zuweisungsoperator verbieten.
  Tile(const Tile &t);
  void operator=(const Tile &t);

  /**
   * Konstruktor für Kind-Tiles.
   * @param parent Eltern-Tile
   * @param direction Quadrant des Eltern-Tiles, in dem dieses Tile liegt
   * @param roughness Rauheits-Faktor für die weitere Verfeinerung
   * @param north Zeiger auf nördlichen Nachbarn (oder NULL)
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
   * Initialisierungsfunktion für das Wurzel-Tile. Setzt die anfänglichen
   * Zufallswerte und berechnet x- und z-Koordinaten aller Vertices vor.
   */
  void Init(float roughness);
  /**
   * Initialisierungsfunktion für ein Kind-Tile. Übernimmt die Werte aus dem
   * entsprechenden Quadranten des Eltern-Tiles und berechnet x- und z-
   * Koordinaten aller Vertices vor.
   */
  void InitFromParent(void);
  /**
   * Führt die Verfeinerung der Höheninformationen nach dem
   * Diamond-Square-Algorithmus zur Blockgröße block_size durch.
   */
  void Refine(int block_size, float roughness);
  /**
   * Initialisiert die Kind-Tiles, falls nötig.
   * @param roughness Rauheits-Faktor für die Verfeinerung der Kind-Tiles
   * @param north Nördlicher Nachbar dieses Tiles
   * @param west Westlicher Nachbar dieses Tiles
   */
  void InitChildren(float roughness, Tile *north, Tile *west);

  /**
   * "Repariert" die Übergänge des Tiles. Übernimmt die südlichste Zeile des
   * nördlichen Nachbarn und östlichste Spalte des westlichen Nachbarn.
   */
  void FixEdges(Tile *north, Tile *west);

  /**
   * Reserviert Speicher für den Index Buffer.
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
   * Größe des Tiles (Seitenlänge)
   */
  const int size_;
  /**
   * Anzahl zusätzlicher LOD-Ebenen unter diesem Tile
   */
  const int num_lod_;

  /**
   * Feld der Vertices dieses Tiles
   */
  Vector *vertices_;
  /**
   * Indizes für die Triangulierung des Höhenfeldes
   * @see Tile::TriangulateLines
   * @see Tile::TriangulateZOrder
   */
  unsigned int *index_buffer_;
  /**
   * Übergeordnetes Eltern-Tile
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
