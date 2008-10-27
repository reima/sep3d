#pragma once
#include "stdafx.h"
#include <string>

/**
 * H�henfeld-Tile
 */
class Tile {
  enum Direction { NW = 0, NE, SW, SE };

 public:
  /**
   * Konstruktor.
   * @param n Legt die Gr��e des Tiles (2^lod - 1) fest
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
   * Ermittelt die minimale H�he im Tile und gibt sie zur�ck.
   */
  float GetMinHeight() const;

  /**
   * Ermittelt die maximale H�he im Tile und gibt sie zur�ck.
   */
  float GetMaxHeight() const;

  /**
   * Speichert die H�hendaten in mehrere (farbige) Bitmaps.
   */
  void SaveImages(const std::wstring &filename) const;
  
  /**
   * Trianguliert streifenweise.
   */
  void TriangulateLines(void);

  /**
   * Trianguliert mit Z-Order.
   */
  void TriangulateZOrder(void);

  /**
   * Speichert H�henfeld im OBJ-3D-Format.
   */
  void SaveObj(const std::wstring &filename) const;

 private:
  // Kopierkonstruktor und Zuweisungsoperator verbieten.
  Tile(const Tile &t);
  void operator=(const Tile &t);

  Tile(Tile *parent, Direction direction, float roughness, Tile *north,
       Tile *west);

  void SaveImageTilesForLOD(unsigned char *image_data, int image_size,
                            int x_off, int y_off, int lod) const;
  void SaveObj0(const std::wstring &basename,
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
   * F�hrt die Verfeinerung der H�heninformationen nach dem Diamond-Square-Alg.
   * zur Blockgr��e block_size durch.
   */
  void Refine(int block_size, float roughness);

  void InitChildren(float roughness, Tile *north, Tile *west);

  void FixEdges(Tile *north, Tile *west);

  void InitIndexBuffer(void);
  void TriangulateZOrder0(int x1, int y1, int x2, int y2, int &i);

  const int lod_;
  const int size_;
  const int num_lod_;

  struct Vector { float x, y, z; };

  Vector *vertices_;
  unsigned int *index_buffer_;

  Tile *parent_;
  Direction direction_;
  Tile *children_[4];
};
