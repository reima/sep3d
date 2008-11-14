#pragma once
#include "LightSource.h"

class PointLight : public LightSource {
 public:
  PointLight(D3DXVECTOR3 &position, D3DXVECTOR3 &color);
  ~PointLight(void);
  virtual void OnFrameMove(float elapsed_time);

  static void GetHandles(ID3D10Effect *effect);  
 
 private:
  D3DXVECTOR3 position_;

  static unsigned int instance_count;
  static ID3D10EffectVectorVariable *pPos;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumPL;
};
