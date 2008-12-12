#include "FixedLODSelector.h"
#include "Tile.h"

bool FixedLODSelector::IsLODSufficient(const Tile *tile,
                                       const CBaseCamera *camera) const {
  return tile->GetLOD() == lod_;
}

FixedLODSelector::FixedLODSelector(int lod) : lod_(lod) {
}
