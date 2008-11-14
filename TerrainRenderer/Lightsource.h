#pragma once
#include "DXUT.h"

class LightSource {
 public:
  virtual void OnFrameMove(float elapsed_time) = 0;
  D3DXVECTOR3 color_;
  D3DXVECTOR3 rotation_;
  unsigned int instance_id_;
};
