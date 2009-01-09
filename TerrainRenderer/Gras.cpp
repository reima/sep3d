#include "Gras.h"
#define voide void

ID3D10InputLayout* Gras::vertex_layout_ = NULL;
ID3D10EffectTechnique* Gras::technique_ = NULL;
ID3D10EffectShaderResourceVariable* Gras::texture_ev_ = NULL;
ID3D10ShaderResourceView* Gras::texture_srv_ = NULL;

Gras::Gras(void)
    : seeds_buffer_(NULL),
      device_(NULL) {
}

Gras::~Gras(void) {
  ReleaseBuffers();
}

void Gras::ReleaseBuffers(void) {
  SAFE_RELEASE(seeds_buffer_);
}

void Gras::PlaceSeed(const D3DXVECTOR3 &position,
                     float normalized_height,
                     const D3DXVECTOR3 &normal) {
  if (normalized_height < 0.05) return;
  seeds_.push_back(position);
}

HRESULT Gras::CreateStaticBuffers(ID3D10Device *device) {
  HRESULT hr;
  ReleaseStaticBuffers();
  
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(device,
      L"Textures\\Billboards\\GrassPack.dds", NULL, NULL,
      &texture_srv_, NULL));

  return S_OK;
}

void Gras::ReleaseStaticBuffers(void) {
  SAFE_RELEASE(vertex_layout_);
  SAFE_RELEASE(texture_srv_);
}

void Gras::GetStaticShaderHandles(ID3D10Device *device,
                                  ID3D10Effect *effect) {
  technique_ = effect->GetTechniqueByName("Grass");
  texture_ev_ = effect->GetVariableByName("g_tGrass")->AsShaderResource();

  const D3D10_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };
  UINT num_elements = sizeof(layout) / sizeof(layout[0]);
  D3D10_PASS_DESC pass_desc;
  technique_->GetPassByIndex(0)->GetDesc(&pass_desc);
  device->CreateInputLayout(layout, num_elements, pass_desc.pIAInputSignature,
                            pass_desc.IAInputSignatureSize, &vertex_layout_);

  texture_ev_->SetResource(texture_srv_);
}

HRESULT Gras::CreateBuffers(ID3D10Device *device) {
  HRESULT hr;
  ReleaseBuffers();

  device_ = device;

  if (seeds_.size() == 0) {
    seeds_buffer_ = NULL;
    return S_OK;
  }

  D3D10_BUFFER_DESC buffer_desc;
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(D3DXVECTOR3) * seeds_.size();
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = &seeds_[0];
  init_data.SysMemPitch = 0;
  init_data.SysMemSlicePitch = 0;
  V_RETURN(device_->CreateBuffer(&buffer_desc, &init_data, &seeds_buffer_));

  return S_OK;
}

void Gras::GetShaderHandles(ID3D10Effect *effect) {  
}

void Gras::Draw(void) {
  if (seeds_buffer_ == NULL) return;
  UINT stride = sizeof(D3DXVECTOR3);
  UINT offset = 0;
  device_->IASetVertexBuffers(0, 1, &seeds_buffer_, &stride, &offset);

  // Primitivtyp setzen
  device_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
  // Vertex Layout setzen
  device_->IASetInputLayout(vertex_layout_);
  
  D3D10_TECHNIQUE_DESC tech_desc;
  technique_->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    technique_->GetPassByIndex(p)->Apply(0);
    device_->Draw(seeds_.size(),0);
  }


}
