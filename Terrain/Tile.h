#pragma once
#include "stdafx.h"

/**
 * Höhenfeld-Tile
 */
class Tile {
 public:
  /**
   * Konstruktor.
   * @param lod Level of Detail, legt die Größe des Tiles (2^lod - 1) fest
   * @param roughness Rauheits-Faktor (je höher desto größer die
   *                  Höhenunterschiede)
   */
  Tile(int lod, float roughness);
  Tile(const Tile &t);
  ~Tile(void);

  /** 
   * Gibt die Auflösung (Seitenlänge) des Tiles zurück.
   */
  int getSize() const { return size_; }

  /**
   * Ermittelt die minimale Höhe im Tile und gibt sie zurück.
   */
  float getMinHeight() const;

  /**
   * Ermittelt die maximale Höhe im Tile und gibt sie zurück.
   */
  float getMaxHeight() const;

  /**
   * Speichert die Höhendaten in ein Graustufenbitmap.
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
  unsigned int *index_buffer_;  
};
