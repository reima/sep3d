#pragma once
#ifndef TILE_H
#define TILE_H
#include <string>
#include "DXUT.h"

class LODSelector;

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
  int GetResolution(void) const { return size_ * size_; }

  /**
   * Gibt die Detailstufe (LOD) des Tiles zurück.
   */
  int GetLOD(void) const { return lod_; }

  /**
   * Ermittelt die minimale Höhe im Tile und gibt sie zurück.
   */
  float GetMinHeight(void) const;

  /**
   * Ermittelt die maximale Höhe im Tile und gibt sie zurück.
   */
  float GetMaxHeight(void) const;

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

  /**
   * Erzeugt Vertex-, Normalen- und Index-Buffer lädt das Dreieckgitter des
   * Tiles in diese hoch. Wird ggf. rekursiv für alle Kinder des Tiles
   * aufgerufen.
   * @warning Das Tile muss zuvor trianguliert worden sein (durch Aufruf von
   *          Tile::TriangulateLines oder Tile::TriangulateZOrder).
   * @see Tile::ReleaseBuffers
   */
  HRESULT CreateBuffers(ID3D10Device *pd3dDevice);

  /**
   * Gibt das Tile auf dem D3D10-Gerät aus.
   * @param pd3dDevice Das D3D10-Gerät.
   * @param lod_selector Ein LODSelector, der bestimmt, ob die LOD-Stufe des
   *                     Tiles ausreicht. Wenn nicht, werden rekursiv die
   *                     Kinder des Tiles gezeichnet.
   * @param cam_pos Die Position der Kamera, zur Übergabe an den LODSelector.
   * @warning Vor dem Aufruf müssen die D3D10-Buffer mit Tile::CreateBuffers
   *          erzeugt werden.
   */
  void Draw(ID3D10Device *pd3dDevice, LODSelector *lod_selector,
            const D3DXVECTOR3 *cam_pos) const;

  /**
   * Gibt den für die interne Darstellung reservierten Speicher frei (auch
   * rekursiv für alle Kind-Tiles).
   * @warning Die Methoden Tile::GetMinHeight, Tile::GetMaxHeight,
   *          Tile::SaveObjs, Tile::CreateBuffers werden danach _nicht_ mehr
   *          funktionieren. Tile::Draw wird unabhängig vom Aufruf dieser
   *          Methode weiterarbeiten, sofern vorher Tile::CreateBuffers
   *          aufgerufen wurde.
   */
  void FreeMemory(void);

  /**
   * Berechnet die Normalen des Terrains.
   */
  void CalculateNormals(void);

  void GetBoundingBox(D3DXVECTOR3 *out, D3DXVECTOR3 *mid);

 private:
  /**
   * Richtungstyp, der einen Quadranten eines Tiles spezifiziert
   */
  enum Direction { NW = 0, NE, SW, SE };

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
   * Rekursive Implementierung von CalculateNormals
   */
  void CalculateNormals0(Tile *north, Tile *west);

  /**
   * Normalisiert alle Vertex-Normalen.
   */
  void NormalizeNormals(void);

  /**
   * Reserviert Speicher für den Index Buffer.
   */
  void InitIndexBuffer(void);

  /**
   * Rekursive Implementierung der Z-Order-Triangulierung.
   */
  void TriangulateZOrder0(int x1, int y1, int x2, int y2, int &i);

  /**
   * Gibt die von CreateBuffers erzeugten Buffer wieder frei (nicht rekursiv).
   */
  void ReleaseBuffers(void);


  void CalculateHeights(void);

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
  D3DXVECTOR3 *vertices_;
  /**
   * Indizes für die Triangulierung des Höhenfeldes
   * @see Tile::TriangulateLines
   * @see Tile::TriangulateZOrder
   */
  unsigned int *indices_;
  /**
   * Feld der Per-Vertex-Normalen dieses Tiles
   */
  D3DXVECTOR3 *vertex_normals_;
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

  /**
   * Zeiger auf den D3D10-Vertex-Buffer.
   */
  ID3D10Buffer *vertex_buffer_;
  /**
   * Zeiger auf den D3D10-Normalen-Buffer.
   */
  ID3D10Buffer *normal_buffer_;
  /**
   * Zeiger auf den D3D10-Index-Buffer.
   */
  ID3D10Buffer *index_buffer_;

  float max_height_;
  float min_height_;
};

#endif
