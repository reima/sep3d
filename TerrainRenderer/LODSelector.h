#pragma once
#include "DXUT.h"

class Tile;

/**
 * Strategie für die Bestimmung der anzuzeigenden LOD-Stufe.
 */
class LODSelector {
 public:
  /**
   * Bestimmt, ob die LOD-Stufe eines bestimmten Tiles, das von einer
   * bestimmten Kameraposition aus betrachtet wird, ausreichend ist.
   */
  virtual bool IsLODSufficient(const Tile *tile,
                               const D3DXVECTOR3 *cam_pos) const = 0;
};
