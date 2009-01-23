#pragma once
#include "ParticleEmitter.h"

class PointEmitter : public ParticleEmitter {
 public:
  PointEmitter(const D3DXVECTOR3 &pos,
               const D3DXVECTOR3 &dir,
               float spread, UINT num);
  virtual ~PointEmitter(void);

 protected:
  virtual void GetShaderHandles0(ID3D10Effect *effect,
                                 ID3D10EffectTechnique *technique);
  virtual void SetShaderVariables(void);

 private:
  D3DXVECTOR3 position_;
  D3DXVECTOR3 direction_;
  float spread_;

  ID3D10EffectVectorVariable *position_ev_;
  ID3D10EffectVectorVariable *direction_ev_;
  ID3D10EffectScalarVariable *spread_ev_;
};
