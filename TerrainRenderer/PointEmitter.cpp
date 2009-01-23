#include "PointEmitter.h"

PointEmitter::PointEmitter(const D3DXVECTOR3 &pos,
                           const D3DXVECTOR3 &dir,
                           float spread, UINT num)
      : ParticleEmitter(num),
        position_(pos),
        direction_(dir),
        spread_(spread) {  
}

PointEmitter::~PointEmitter(void) {
}

void PointEmitter::GetShaderHandles0(ID3D10Effect *effect,
                                     ID3D10EffectTechnique *technique) {
  position_ev_ = effect->GetVariableByName("g_vPEPosition")->AsVector();
  direction_ev_ = effect->GetVariableByName("g_vPEDirection")->AsVector();
  spread_ev_ = effect->GetVariableByName("g_fPESpread")->AsScalar();
}

void PointEmitter::SetShaderVariables(void) {
  position_ev_->SetFloatVector(position_);
  direction_ev_->SetFloatVector(direction_);
  spread_ev_->SetFloat(spread_);
}