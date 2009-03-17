#pragma once
#include "ParticleEmitter.h"

class PointEmitter : public ParticleEmitter {
 public:
  PointEmitter(const D3DXVECTOR3 &pos,
               const D3DXVECTOR3 &dir,
               float spread, UINT num);
  virtual ~PointEmitter(void);

  void SetPosition(const D3DXVECTOR3 &pos) { position_ = pos; }
  const D3DXVECTOR3 *GetPosition(void) const { return &position_; }

  void SetDirection(const D3DXVECTOR3 &dir);  

 protected:
  virtual void GetShaderHandles0(ID3D10Effect *effect);
  virtual void SetShaderVariables(void);

 private:
  D3DXVECTOR3 position_;
  D3DXMATRIX transform_;
  float spread_;

  ID3D10EffectVectorVariable *position_ev_;
  ID3D10EffectMatrixVariable *transform_ev_;
  ID3D10EffectScalarVariable *spread_ev_;
};
