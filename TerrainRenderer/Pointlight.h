#pragma once
#include "LightSource.h"

class PointLight : public LightSource {
 public:
  PointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
             const D3DXVECTOR3 &rotation);
  static void GetHandles(ID3D10Effect *effect);
  virtual void OnFrameMove(float elapsed_time);

 private:
  D3DXVECTOR3 position_;
  static unsigned int instance_count;
  static ID3D10EffectVectorVariable *pPos;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumPL;
};
