#pragma once
#include "DXUT.h"

class LightSource {
 public:
  LightSource(const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation)
      : color_(color), rotation_(rotation) {};
  virtual void OnFrameMove(float elapsed_time) = 0;
 protected:
  D3DXVECTOR3 color_;
  D3DXVECTOR3 rotation_;
  unsigned int instance_id_;
};
