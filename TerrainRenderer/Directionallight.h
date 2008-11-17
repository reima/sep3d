#pragma once
#include "LightSource.h"

class DirectionalLight : public LightSource {
 public:
  DirectionalLight(const D3DXVECTOR3 &direction, const D3DXVECTOR3 &color,
                   const D3DXVECTOR3 &rotation);
  static void GetHandles(ID3D10Effect *effect);
  virtual void OnFrameMove(float elapsed_time);

 private:
  D3DXVECTOR3 direction_;
  static unsigned int instance_count;  
  static ID3D10EffectVectorVariable *pDir;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumDL;
};
