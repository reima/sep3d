#include <ctime>
#include "ParticleEmitter.h"

ID3D10Texture2D *ParticleEmitter::random_tex_ = NULL;
ID3D10ShaderResourceView *ParticleEmitter::random_srv_ = NULL;
ID3D10EffectShaderResourceVariable* ParticleEmitter::random_ev_ = NULL;

ParticleEmitter::ParticleEmitter(UINT num_particles)
    : num_particles_(num_particles),
      technique_(NULL),
      elapsed_time_ev_(NULL),
      input_layout_(NULL),
      device_(NULL),
      first_step_(true) {
  particle_buffers_[0] = NULL;
  particle_buffers_[1] = NULL;
}

ParticleEmitter::~ParticleEmitter(void) {
  SAFE_RELEASE(particle_buffers_[0]);
  SAFE_RELEASE(particle_buffers_[1]);
  SAFE_RELEASE(input_layout_);

  std::vector<BOUND_RESOURCE>::iterator it;
  for (it = resources_.begin(); it != resources_.end(); ++it) {
    SAFE_RELEASE(it->second);
  }
}

HRESULT ParticleEmitter::CreateBuffers(ID3D10Device *device) {
  HRESULT hr;
  device_ = device;

  D3D10_BUFFER_DESC buffer_desc;
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(PARTICLE) * num_particles_;
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_STREAM_OUTPUT;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;

  PARTICLE *data = new PARTICLE[num_particles_];
  start_particles_ = InitParticles(data);
  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = data;
  init_data.SysMemPitch = 0;
  init_data.SysMemSlicePitch = 0;

  V_RETURN(device->CreateBuffer(&buffer_desc, &init_data, &particle_buffers_[0]));
  V_RETURN(device->CreateBuffer(&buffer_desc, NULL, &particle_buffers_[1]));
  delete[] data;

  if (random_tex_ == NULL) {
    CreateRandomTexture(device);
  }

  V_RETURN(CreateBuffers0(device));

  return S_OK;
}

void ParticleEmitter::GetShaderHandles(ID3D10Effect *effect) {
  assert(device_ != NULL);
  technique_ = GetTechnique(effect);

  const D3D10_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "MAXAGE",   0, DXGI_FORMAT_R32_FLOAT,       0, 28, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "SIZE",     0, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "ROTATION", 0, DXGI_FORMAT_R32_FLOAT,       0, 36, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TYPE",     0, DXGI_FORMAT_R32_UINT,        0, 40, D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };

  UINT num_elements = sizeof(layout) / sizeof(layout[0]);
  D3D10_PASS_DESC pass_desc;
  technique_->GetPassByIndex(0)->GetDesc(&pass_desc);
  device_->CreateInputLayout(layout, num_elements, pass_desc.pIAInputSignature,
                             pass_desc.IAInputSignatureSize, &input_layout_);

  elapsed_time_ev_ = effect->GetVariableByName("g_fElapsedTime")->AsScalar();
  
  if (random_ev_ == NULL) {
    random_ev_ = effect->GetVariableByName("g_tRandom")->AsShaderResource();
  }

  GetShaderHandles0(effect);
}

void ParticleEmitter::ReleaseResources(void) {
  SAFE_RELEASE(random_srv_);
  SAFE_RELEASE(random_tex_);
}

void ParticleEmitter::SimulationStep(float elapsed_time) {
  UINT stride = sizeof(PARTICLE);
  UINT offset = 0;
  device_->IASetVertexBuffers(0, 1, &particle_buffers_[0], &stride, &offset);
  device_->SOSetTargets(1, &particle_buffers_[1], &offset);
  device_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
  device_->IASetInputLayout(input_layout_);

  elapsed_time_ev_->SetFloat(elapsed_time);
  random_ev_->SetResource(random_srv_);
  SetShaderVariables();

  D3D10_TECHNIQUE_DESC tech_desc;
  technique_->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    technique_->GetPassByIndex(p)->Apply(0);
    if (first_step_) {
      device_->Draw(start_particles_, 0);
      first_step_ = false;
    } else {
      device_->DrawAuto();
    }
  }

  ID3D10Buffer *no_buffer = NULL;
  device_->SOSetTargets(1, &no_buffer, &offset);

  // Switch buffers
  ID3D10Buffer *tmp = particle_buffers_[0];
  particle_buffers_[0] = particle_buffers_[1];
  particle_buffers_[1] = tmp;
}

void ParticleEmitter::Draw(ID3D10EffectTechnique *technique) {
  UINT stride = sizeof(PARTICLE);
  UINT offset = 0;
  device_->IASetVertexBuffers(0, 1, &particle_buffers_[0], &stride, &offset);
  device_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
  device_->IASetInputLayout(input_layout_);

  std::vector<BOUND_RESOURCE>::const_iterator it;
  for (it = resources_.begin(); it != resources_.end(); ++it) {
    it->first->SetResource(it->second);
  }

  D3D10_TECHNIQUE_DESC tech_desc;
  technique->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    technique->GetPassByIndex(p)->Apply(0);
    device_->DrawAuto();
  }
}

HRESULT ParticleEmitter::AddResource(ID3D10Effect *effect,
                                     LPCSTR effect_variable,
                                     LPCWSTR resource_path) {
  assert(device_ != NULL);
  HRESULT hr;

  ID3D10EffectShaderResourceVariable *variable =
    effect->GetVariableByName(effect_variable)->AsShaderResource();
  ID3D10ShaderResourceView *srv = NULL;
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(device_, resource_path,
    NULL, NULL, &srv, NULL));

  resources_.push_back(BOUND_RESOURCE(variable, srv));

  return S_OK;
}

HRESULT ParticleEmitter::CreateRandomTexture(ID3D10Device *device) {
  HRESULT hr;
  const UINT size = 1024;
  srand(static_cast<unsigned int>(time(0)));
  float *data = new float[size*size];
  for (UINT i = 0; i < size*size; ++i) {
    data[i] = (float)rand() / RAND_MAX;
  }
  
  D3D10_TEXTURE2D_DESC tex_desc;
  tex_desc.Width = size;
  tex_desc.Height = size;
  tex_desc.MipLevels = 1;
  tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
  tex_desc.Usage = D3D10_USAGE_IMMUTABLE;
  tex_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  tex_desc.CPUAccessFlags = 0;
  tex_desc.MiscFlags = 0;
  tex_desc.ArraySize = 1;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;

  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = data;
  init_data.SysMemPitch = size * sizeof(float);
  init_data.SysMemSlicePitch = size * sizeof(float);

  V_RETURN(device->CreateTexture2D(&tex_desc, &init_data, &random_tex_));
  delete[] data;

  D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
  ZeroMemory(&srv_desc, sizeof(srv_desc));
  srv_desc.Format = tex_desc.Format;
  srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
  srv_desc.Texture2D.MostDetailedMip = 0;
  V_RETURN(device->CreateShaderResourceView(random_tex_, &srv_desc, &random_srv_));

  return S_OK;
}