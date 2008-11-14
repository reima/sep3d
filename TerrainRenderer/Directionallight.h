#pragma once
#include "LightSource.h"

class DirectionalLight : public LightSource {
 public:
  DirectionalLight(D3DXVECTOR3 &direction, D3DXVECTOR3 &color);
  ~DirectionalLight(void);
  virtual void OnFrameMove(float elapsed_time);

  static void GetHandles(ID3D10Effect *effect);  

 private:
  D3DXVECTOR3 direction_;

  static unsigned int instance_count;  
  static ID3D10EffectVectorVariable *pDir;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumDL;
};
