#pragma once
#include "ParticleEmitter.h"

class BoxEmitter : public ParticleEmitter {
 public:
  BoxEmitter(const D3DXVECTOR3 &min_pos,
             const D3DXVECTOR3 &max_pos,
             const D3DXVECTOR3 &velocity,
             UINT num);
  virtual ~BoxEmitter(void);

 protected:
  virtual void GetShaderHandles0(ID3D10Effect *effect);
  virtual void SetShaderVariables(void);

 private:
  D3DXVECTOR3 min_pos_;
  D3DXVECTOR3 max_pos_;
  D3DXVECTOR3 velocity_;
  ID3D10EffectVectorVariable *min_pos_ev_;
  ID3D10EffectVectorVariable *max_pos_ev_;
  ID3D10EffectVectorVariable *velocity_ev_;
};
