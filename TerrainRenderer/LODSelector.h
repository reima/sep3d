#pragma once
#include "DXUT.h"

class Tile;

class LODSelector {
 public:
  virtual bool IsLODSufficient(const Tile *tile,
                               const D3DXVECTOR3 *cam_pos) const = 0;
};
