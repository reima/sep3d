#pragma once
#include "LODSelector.h"

class DynamicLODSelector : public LODSelector {
 public:
  DynamicLODSelector(float fov_y, int screen_height, float max_error);
  ~DynamicLODSelector(void);

  virtual bool IsLODSufficient(const Tile *tile,
                               const CBaseCamera *camera) const;

 private:
  /**
   * delta = factor_ * tau * z;
   * delta: World Space Error
   * tau: Auf View Plan proj. World Space Error
   */
  float factor_;
};
