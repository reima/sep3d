#pragma once
#include "BoxEmitter.h"

class RainEmitter :  public BoxEmitter {
 public:
  RainEmitter(const D3DXVECTOR3 &min_pos,
             const D3DXVECTOR3 &max_pos,
             UINT num);
  virtual ~RainEmitter(void);
  virtual void Draw(void);

 protected:
  virtual ID3D10EffectTechnique *GetTechnique(ID3D10Effect *effect);
  virtual void GetShaderHandles0(ID3D10Effect *effect);
  virtual UINT InitParticles(PARTICLE *particles);

 private:
  ID3D10EffectTechnique *draw_technique_;
};
