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
   * @param lod Level of Detail, legt die Gr��e des Tiles (2^lod - 1) fest
   * @param roughness Rauheits-Faktor (je h�her desto gr��er die
   *                  H�henunterschiede)
   * @param num_lod Anzahl zus�tzlicher LOD-Ebenen
   */
  Tile(int lod, float roughness, int num_lod);
  ~Tile(void);

  /** 
   * Gibt die Aufl�sung (Seitenl�nge) des Tiles zur�ck.
   */
  int size() const { return size_; }

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
   * Speichert Terrain-3D-Modell im OBJ-Format.
   */
  void SaveObj(const std::wstring &filename) const;

 private:
  Tile(const Tile &t);
  Tile(Tile *parent, Direction direction, float roughness, Tile *north,
       Tile *west);

  void SaveImage0(const std::wstring &basename, const std::wstring &extension,
                  float min, float max) const;
  void SaveObj0(const std::wstring &basename,
                const std::wstring &extension) const;

  void Init(float roughness);
  void Refine(int block_size, float roughness);
  void InitChildren(float roughness, Tile *north, Tile *west);
  void InitFromParent(void);
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
