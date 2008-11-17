#pragma once
#include "LightSource.h"

class SpotLight : public LightSource {
 public:
  SpotLight(D3DXVECTOR3 &position, D3DXVECTOR3 &direction, D3DXVECTOR3 &color,
            float cutoff_angle, float exponent);
  ~SpotLight(void);

  virtual void OnFrameMove(float elapsed_time);

  static void GetHandles(ID3D10Effect *effect);  
 
 private:
  D3DXVECTOR3 position_;

  static unsigned int instance_count;
  static ID3D10EffectVectorVariable *pPos;
  static ID3D10EffectVectorVariable *pDir;
  static ID3D10EffectVectorVariable *pAngleExp;
  static ID3D10EffectVectorVariable *pColor;
  static ID3D10EffectScalarVariable *pNumSL;
};
