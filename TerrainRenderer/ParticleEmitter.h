#pragma once
#include <vector>
#include "DXUT.h"

class ParticleEmitter {
 public:
  ParticleEmitter(UINT num_particles);
  virtual ~ParticleEmitter(void);
  HRESULT CreateBuffers(ID3D10Device *device);
  void GetShaderHandles(ID3D10Effect *effect,
                        ID3D10EffectTechnique *technique);
  void SimulationStep(float elapsed_time);
  void Draw(ID3D10EffectTechnique *technique);

  static void ReleaseResources(void);

 protected:
  virtual HRESULT CreateBuffers0(ID3D10Device *device) { return S_OK; }
  virtual void GetShaderHandles0(ID3D10Effect *effect,
                                 ID3D10EffectTechnique *technique) = 0;
  virtual void SetShaderVariables(void) = 0;
  HRESULT AddResource(ID3D10Effect *effect,
                      LPCSTR effect_variable,
                      LPCWSTR resource_path);

  ID3D10Buffer *particle_buffers_[2];
  ID3D10EffectTechnique *technique_;
  ID3D10EffectScalarVariable *elapsed_time_ev_;
  ID3D10InputLayout *input_layout_;
  ID3D10Device *device_;
  UINT num_particles_;

  struct PARTICLE {
    D3DXVECTOR3 position;
    D3DXVECTOR3 velocity;
    float life, max_life;
    float size;
    float rotation;
    UINT type;
  };

 private:
  static HRESULT CreateRandomTexture(ID3D10Device *device);

  static ID3D10Texture1D *random_tex_;
  static ID3D10ShaderResourceView *random_srv_;
  static ID3D10EffectShaderResourceVariable *random_ev_;

  typedef std::pair<ID3D10EffectShaderResourceVariable *, ID3D10ShaderResourceView *> BOUND_RESOURCE;
  std::vector<BOUND_RESOURCE> resources_;
  bool first_step_;
};