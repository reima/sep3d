#include "RainEmitter.h"

RainEmitter::RainEmitter(const D3DXVECTOR3 &min_pos,
                         const D3DXVECTOR3 &max_pos,
                         UINT num)
    : BoxEmitter(min_pos, max_pos, D3DXVECTOR3(0, -1, 0), num) {
}

RainEmitter::~RainEmitter(void) {
}

void RainEmitter::GetShaderHandles0(ID3D10Effect *effect) {
  BoxEmitter::GetShaderHandles0(effect);
  draw_technique_ = effect->GetTechniqueByName("RenderRainBillboard");
  AddResource(effect, "g_tRainDrop", L"Textures\\Billboards\\raindrop.png");
}

ID3D10EffectTechnique *RainEmitter::GetTechnique(ID3D10Effect *effect) {
  return effect->GetTechniqueByName("RainSim");
}

void RainEmitter::Draw(void) {
  ParticleEmitter::Draw(draw_technique_);
}

UINT RainEmitter::InitParticles(PARTICLE *particles) {
  for (UINT i = 0; i < num_particles_; ++i) {
    particles[i].type = (UINT)-1;
    particles[i].age = ((float)rand() / RAND_MAX)*(-5.0f);
  }
  return num_particles_;
}