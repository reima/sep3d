#pragma once
#include "stdafx.h"

/**
 * H�henfeld-Tile
 */
class Tile {
 public:
  /**
   * Konstruktor.
   * @param lod Level of Detail, legt die Gr��e des Tiles (2^lod - 1) fest
   * @param roughness Rauheits-Faktor (je h�her desto gr��er die
   *                  H�henunterschiede)
   */
  Tile(int lod, float roughness);
  Tile(const Tile &t);
  ~Tile(void);

  /** 
   * Gibt die Aufl�sung (Seitenl�nge) des Tiles zur�ck.
   */
  int getSize() const { return size_; }

  /**
   * Ermittelt die minimale H�he im Tile und gibt sie zur�ck.
   */
  float getMinHeight() const;

  /**
   * Ermittelt die maximale H�he im Tile und gibt sie zur�ck.
   */
  float getMaxHeight() const;

  /**
   * Speichert die H�hendaten in ein Graustufenbitmap.
   */
  void saveImage(const TCHAR *filename) const;
  
  /**
   * Trianguliert streifenweise
   */
  void triangulateLines(void);

  /**
   * Trianguliert mit Z-Order
   */
  void triangulateZOrder(void);

  /**
   * Speichert 
   */
  void saveObj(const TCHAR *filename) const;

 private:
  void init(float roughness);
  void initIndexBuffer(void);
  void z_rec(int x1, int y1, int x2, int y2, int &i);

  const int lod_;
  const int size_;
  float *height_map_;
  int *index_buffer_;  
};