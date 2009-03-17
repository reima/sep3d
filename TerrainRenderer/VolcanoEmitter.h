#pragma once
#include "PointEmitter.h"

class VolcanoEmitter : public PointEmitter {
 public:
  VolcanoEmitter(const D3DXVECTOR3 &pos, const D3DXVECTOR3 &dir, float spread);
  virtual ~VolcanoEmitter(void);
  virtual void Draw(void);

 protected:
  virtual ID3D10EffectTechnique *GetTechnique(ID3D10Effect *effect);
  virtual void GetShaderHandles0(ID3D10Effect *effect);
  virtual UINT InitParticles(PARTICLE *particles);

 private:
  ID3D10EffectTechnique *techniques_[2];
};
