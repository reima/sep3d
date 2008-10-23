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
   * Trianguliert Streifenweise
   */
  void triangulate_lines(void) const;


  /**
   * Trianguliert Z-Order
   */
  void triangulate_z(void) ;
 
 private:
  void init(float roughness);
  void z_rec(int x, int y, int x2, int y2);


  const int lod_;
  const int size_;
  float *height_map_;
  unsigned *index_buffer;
  unsigned buffer_index;

};
