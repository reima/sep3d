#include "BoxEmitter.h"

BoxEmitter::BoxEmitter(const D3DXVECTOR3 &min_pos,
                       const D3DXVECTOR3 &max_pos,
                       const D3DXVECTOR3 &velocity,
                       UINT num)
    : ParticleEmitter(num),
      min_pos_(min_pos),
      max_pos_(max_pos),
      velocity_(velocity) {
}

BoxEmitter::~BoxEmitter(void) {
}

void BoxEmitter::GetShaderHandles0(ID3D10Effect *effect) {
  min_pos_ev_ = effect->GetVariableByName("g_vBEMinVertex")->AsVector();
  max_pos_ev_ = effect->GetVariableByName("g_vBEMaxVertex")->AsVector();
  velocity_ev_ = effect->GetVariableByName("g_vBEVelocity")->AsVector();
}

void BoxEmitter::SetShaderVariables(void) {
  min_pos_ev_->SetFloatVector(min_pos_);
  max_pos_ev_->SetFloatVector(max_pos_);
  velocity_ev_->SetFloatVector(velocity_);
}
