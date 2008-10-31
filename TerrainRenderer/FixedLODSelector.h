#pragma once
#include "LODSelector.h"

class FixedLODSelector : public LODSelector {
 public:
  FixedLODSelector(int lod);
  virtual bool IsLODSufficient(const Tile *tile,
                               const D3DXVECTOR3 *cam_pos) const;
 private:
  const int lod_;
};
