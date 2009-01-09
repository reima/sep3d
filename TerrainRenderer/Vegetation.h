#pragma once
#include "DXUT.h"

class Vegetation {
 public:
  Vegetation(void);
  virtual ~Vegetation(void);

  virtual void PlaceSeed(const D3DXVECTOR3 &position,
                         float normalized_height,
                         const D3DXVECTOR3 &normal) = 0;
  virtual HRESULT CreateBuffers(ID3D10Device *device) = 0;
  virtual void GetShaderHandles(ID3D10Effect *effect) = 0;
  virtual void Draw(void) = 0;
};
