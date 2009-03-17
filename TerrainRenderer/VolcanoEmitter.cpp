#include "VolcanoEmitter.h"

VolcanoEmitter::VolcanoEmitter(const D3DXVECTOR3 &pos,
                               const D3DXVECTOR3 &dir,
                               float spread)
    : PointEmitter(pos, dir, spread, 100000) {
  techniques_[0] = techniques_[1] = NULL;
}

VolcanoEmitter::~VolcanoEmitter(void) {
}

void VolcanoEmitter::GetShaderHandles0(ID3D10Effect *effect) {
  PointEmitter::GetShaderHandles0(effect);

  AddResource(effect, "g_tVulcanoFire", L"Textures\\Billboards\\waterfall0080.png");  
  AddResource(effect, "g_tVulcanoHighlight", L"Textures\\Billboards\\ice.png");

  techniques_[0] = effect->GetTechniqueByName("RenderParticlesBillboardIntense");
  techniques_[1] = effect->GetTechniqueByName("RenderParticlesBillboard");
}

ID3D10EffectTechnique *VolcanoEmitter::GetTechnique(ID3D10Effect *effect) {
  return effect->GetTechniqueByName("VolcanoSim");
}

void VolcanoEmitter::Draw(void) {
  ParticleEmitter::Draw(techniques_[0]);
  ParticleEmitter::Draw(techniques_[1]);
}

UINT VolcanoEmitter::InitParticles(PARTICLE *particles) {
  for (UINT i = 0; i < 5; ++i) {
    particles[i].type = (UINT)-1;
  }
  return 5;
}
