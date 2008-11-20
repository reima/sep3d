#include "Environment.h"

Environment::Environment(ID3D10Device *device)
    : vertex_buffer_(NULL),
      world_view_inverse_mat_(NULL),
      back_buffer_size_(NULL),
      camera_fov_(NULL),
      effect_(NULL) {
  Init(device);
}

Environment::~Environment(void) {
  SAFE_RELEASE(vertex_buffer_);
}

void Environment::Init(ID3D10Device *device) {
  // Vertex Buffer anlegen
  D3D10_BUFFER_DESC buffer_desc;
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(D3DXVECTOR3) * 4;
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;

  D3DXVECTOR3 vertices[] = {
    D3DXVECTOR3(-1.0f,  1.0f, 0.99999f),
    D3DXVECTOR3( 1.0f,  1.0f, 0.99999f),
    D3DXVECTOR3(-1.0f, -1.0f, 0.99999f),
    D3DXVECTOR3( 1.0f, -1.0f, 0.99999f),
  };
  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = vertices;
  init_data.SysMemPitch = 0;
  init_data.SysMemSlicePitch = 0;

  device->CreateBuffer(&buffer_desc, &init_data, &vertex_buffer_);
}

void Environment::GetShaderHandles(ID3D10Effect *effect) {  
  world_view_inverse_mat_ =
      effect->GetVariableByName("g_mWorldViewInv")->AsMatrix();
  back_buffer_size_ =
      effect->GetVariableByName("g_vBackBufferSize")->AsVector();
  camera_fov_ =
      effect->GetVariableByName("g_fCameraFOV")->AsScalar();
  effect_ = effect;
}

void Environment::SetBackBufferSize(UINT width, UINT height) {
  assert(back_buffer_size_ != NULL);
  UINT temp[] = { width, height };
  back_buffer_size_->SetRawValue(temp, 0, sizeof(UINT)*2);
}

void Environment::OnFrameMove(const D3DXMATRIX *world_view_matrix,
                              float camera_fov) {
  assert(world_view_inverse_mat_ != NULL);
  assert(camera_fov_ != NULL);
  D3DXMATRIX mat(*world_view_matrix);
  D3DXMatrixInverse(&mat, NULL, &mat);
  world_view_inverse_mat_->SetMatrix(mat);
  camera_fov_->SetFloat(camera_fov);
}

void Environment::Draw(ID3D10Device *device) {
  assert(vertex_buffer_ != NULL);
  assert(effect_ != NULL);
  UINT stride = sizeof(D3DXVECTOR3);
  UINT offset = 0;
  device->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);
  device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  ID3D10EffectTechnique *tech = effect_->GetTechniqueByName("Environment");
  D3D10_TECHNIQUE_DESC tech_desc;
  tech->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    tech->GetPassByIndex(p)->Apply(0);
    device->Draw(4, 0);
  }
}
