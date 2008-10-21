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
 
 private:
  void init(float roughness);

  const int lod_;
  const int size_;
  float *height_map_;
};
