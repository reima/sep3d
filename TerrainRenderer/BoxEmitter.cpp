#include "BoxEmitter.h"

BoxEmitter::BoxEmitter(const D3DXVECTOR3 &min_pos,
                       const D3DXVECTOR3 &max_pos,
                       UINT num)
    : ParticleEmitter(num),
      min_pos_(min_pos),
      max_pos_(max_pos) {
}

BoxEmitter::~BoxEmitter(void) {
}

void BoxEmitter::GetShaderHandles0(ID3D10Effect *effect,
                                   ID3D10EffectTechnique *technique) {
  min_pos_ev_ = effect->GetVariableByName("g_vBEMinVertex")->AsVector();
  max_pos_ev_ = effect->GetVariableByName("g_vBEMaxVertex")->AsVector();  
}

void BoxEmitter::SetShaderVariables(void) {
  min_pos_ev_->SetFloatVector(min_pos_);
  max_pos_ev_->SetFloatVector(max_pos_);  
}
