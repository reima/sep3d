#include "PointEmitter.h"

PointEmitter::PointEmitter(const D3DXVECTOR3 &pos,
                           const D3DXVECTOR3 &dir,
                           float spread, UINT num)
      : ParticleEmitter(num),
        position_(pos),
        spread_(spread) {  
  SetDirection(dir);
}

PointEmitter::~PointEmitter(void) {
}

void PointEmitter::SetDirection(const D3DXVECTOR3 &dir) {
  D3DXVECTOR3 vec = dir;
  D3DXVec3Normalize(&vec, &vec);
  D3DXVECTOR3 up(-vec.z, vec.x, vec.y); // should never be colinear to vec (hopefully)
  D3DXVECTOR3 origin(0, 0, 0);
  D3DXMatrixLookAtLH(&transform_, &origin, &vec, &up);
  D3DXMatrixInverse(&transform_, NULL, &transform_);
}

void PointEmitter::GetShaderHandles0(ID3D10Effect *effect) {
  position_ev_ = effect->GetVariableByName("g_vPEPosition")->AsVector();
  transform_ev_ = effect->GetVariableByName("g_mPETransform")->AsMatrix();
  spread_ev_ = effect->GetVariableByName("g_fPESpread")->AsScalar();
}

void PointEmitter::SetShaderVariables(void) {
  position_ev_->SetFloatVector(position_);
  transform_ev_->SetMatrix(transform_);
  spread_ev_->SetFloat(spread_);
}