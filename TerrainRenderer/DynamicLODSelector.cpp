#include <algorithm>
#include <cmath>
#include "DynamicLODSelector.h"
#include "Tile.h"

#undef min
#undef max

DynamicLODSelector::DynamicLODSelector(float fov_y,
                                       int screen_height,
                                       float max_error) {
  factor_ = 2 * std::tan(fov_y / 2) * max_error / (float)screen_height;
}

DynamicLODSelector::~DynamicLODSelector(void) {
}

bool DynamicLODSelector::IsLODSufficient(const Tile *tile,
                                         const CBaseCamera *camera) const {
  D3DXVECTOR3 bbox[8];
  tile->GetBoundingBox(bbox, NULL);
  D3DXVec3TransformCoordArray(bbox, sizeof(D3DXVECTOR3),
                              bbox, sizeof(D3DXVECTOR3),
                              camera->GetViewMatrix(), 8);
  float min_z = bbox[0].z;
  for (int i = 1; i < 8; ++i) {
    min_z = std::min(min_z, bbox[i].z);
  }
  float max_world_error = factor_ * min_z;
  return tile->GetWorldError() <= max_world_error;
}
