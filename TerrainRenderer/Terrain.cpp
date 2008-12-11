#include "Terrain.h"
#include "Tile.h"

// Makro, um die Indexberechnungen für das "flachgeklopfte" 2D-Array von
// Vertices zu vereinfachen
#define I(x,y) ((y)*size_+(x))

Terrain::Terrain(int n, float roughness, int num_lod)
    : size_((1 << n) + 1),
      device_(NULL),
      vertex_layout_(NULL),
      vertex_buffer_(NULL),
      index_buffer_(NULL),
      tile_scale_ev_(NULL),
      tile_translate_ev_(NULL),
      technique_(NULL),
      indices_(NULL) {
  tile_ = new Tile(this, n, roughness, num_lod);
}

Terrain::~Terrain(void) {
  SAFE_DELETE(indices_);
  SAFE_DELETE(tile_);
  ReleaseBuffers();
}

void Terrain::InitIndexBuffer(void) {
  if (indices_ == NULL) {
    // (size_-1)^2 Blöcke, pro Block 2 Dreiecke, pro Dreieck 3 Indizes
    indices_ = new unsigned int[(size_-1)*(size_-1)*2*3];
  }
}

void Terrain::TriangulateLines(void) {
  InitIndexBuffer();
  // Dreieck 1 links oben, Dreieck 2 rechts unten
  int i = 0;
  for (int y = 0; y < size_ - 1; y++) {
    for (int x = 0; x < size_ - 1; x++) {
      indices_[i++] = I(x, y);     // 1. Dreieck links oben
      indices_[i++] = I(x, y+1);   // 1. Dreieck links unten
      indices_[i++] = I(x+1, y);   // 1. Dreieck rechts oben
      indices_[i++] = I(x+1, y);   // 2. Dreieck rechts oben
      indices_[i++] = I(x, y+1);   // 2. Dreieck links unten
      indices_[i++] = I(x+1, y+1); // 2. Dreieck rechts unten
    }
  }  
}

void Terrain::TriangulateZOrder(void) {
  InitIndexBuffer();
  int i = 0;
  TriangulateZOrder0(0, 0, size_-1, size_-1, i);
}

void Terrain::TriangulateZOrder0(int x1, int y1, int x2, int y2, int &i){
  if (x1 + 1 == x2) {
    // Rekursionsabbruch, Dreiecke erzeugen
    indices_[i++] = I(x1, y1);     // 1. Dreieck links oben
    indices_[i++] = I(x1, y1+1);   // 1. Dreieck links unten
    indices_[i++] = I(x1+1, y1);   // 1. Dreieck rechts oben
    indices_[i++] = I(x1+1, y1);   // 2. Dreieck rechts oben
    indices_[i++] = I(x1, y1+1);   // 2. Dreieck links unten
    indices_[i++] = I(x1+1, y1+1); // 2. Dreieck rechts unten
  } else {
    int x12 = (x1+x2)/2;
    int y12 = (y1+y2)/2;

    TriangulateZOrder0(x1, y1, x12, y12, i);
    TriangulateZOrder0(x12, y1, x2, y12, i);
    TriangulateZOrder0(x1, y12, x12, y2, i);
    TriangulateZOrder0(x12, y12, x2, y2, i);
  }
}

HRESULT Terrain::CreateBuffers(ID3D10Device *device) {
  assert(indices_ != NULL);
  assert(tile_ != NULL);
  HRESULT hr;
  device_ = device;

  // 2D-Vertices für Unit-Tile berechnen
  D3DXVECTOR2 *vertices = new D3DXVECTOR2[size_*size_];
  int i = 0;
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x, ++i) {
      vertices[i].x = (float)x / (size_ - 1);
      vertices[i].y = (float)y / (size_ - 1);
    }
  }

  // Vertex Buffer anlegen
  D3D10_BUFFER_DESC buffer_desc;
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(D3DXVECTOR2) * size_ * size_;
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = vertices;
  init_data.SysMemPitch = 0;
  init_data.SysMemSlicePitch = 0;
  V_RETURN(device->CreateBuffer(&buffer_desc, &init_data, &vertex_buffer_));
  delete[] vertices;

  // Index Buffer anlegen
  buffer_desc.ByteWidth = sizeof(indices_[0]) * (size_-1)*(size_-1)*2*3;
  buffer_desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
  init_data.pSysMem = indices_;
  V_RETURN(device->CreateBuffer(&buffer_desc, &init_data, &index_buffer_));

  tile_->CalculateNormals(indices_);
  V_RETURN(tile_->CreateBuffers(device));

  return S_OK;
}

void Terrain::ReleaseBuffers(void) {
  SAFE_RELEASE(vertex_buffer_);
  SAFE_RELEASE(index_buffer_);
  SAFE_RELEASE(vertex_layout_);
}

void Terrain::GetShaderHandles(ID3D10Effect *effect) {
  // Vertex Layout festlegen
  D3D10_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };
  UINT num_elements = sizeof(layout) / sizeof(layout[0]);
  D3D10_PASS_DESC pass_desc;
  effect->GetTechniqueByName("PhongShading")->GetPassByIndex(0)->GetDesc(&pass_desc);
  device_->CreateInputLayout(layout, num_elements, pass_desc.pIAInputSignature,
                             pass_desc.IAInputSignatureSize, &vertex_layout_);

  tile_scale_ev_ = effect->GetVariableByName("g_fTileScale")->AsScalar();
  tile_translate_ev_ =
      effect->GetVariableByName("g_vTileTranslate")->AsVector();
  tile_heightmap_ev_ =
      effect->GetVariableByName("g_tTerrain")->AsShaderResource();
  terrain_size_ev_ =
      effect->GetVariableByName("g_uiTerrainSize")->AsScalar();
  terrain_size_ev_->SetInt(size_);
}

void Terrain::GetBoundingBox(D3DXVECTOR3 *out, D3DXVECTOR3 *mid) const {
  tile_->GetBoundingBox(out, mid);
}

void Terrain::Draw(ID3D10EffectTechnique *technique, LODSelector *lod_selector,
                   const D3DXVECTOR3 *cam_pos) {
  assert(vertex_buffer_ != NULL);
  assert(index_buffer_ != NULL);
  assert(vertex_layout_ != NULL);
  assert(device_ != NULL);

  // Vertex Buffer setzen
  UINT stride = sizeof(D3DXVECTOR2);
  UINT offset = 0;
  device_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);
  // Index Buffer setzen
  device_->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);
  // Primitivtyp setzen
  device_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  // Vertex Layout setzen
  device_->IASetInputLayout(vertex_layout_);

  technique_ = technique;
  tile_->Draw(lod_selector, cam_pos);
  technique_ = NULL;
}

void Terrain::DrawTile(float scale, D3DXVECTOR2 &translate,
                       ID3D10ShaderResourceView *srv) {
  assert(tile_scale_ev_ != NULL);
  assert(tile_translate_ev_ != NULL);
  assert(tile_heightmap_ev_ != NULL);
  assert(device_ != NULL);
  tile_scale_ev_->SetFloat(scale);
  tile_translate_ev_->SetFloatVector(translate);
  tile_heightmap_ev_->SetResource(srv);
  
  D3D10_TECHNIQUE_DESC tech_desc;
  technique_->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    technique_->GetPassByIndex(p)->Apply(0);
    device_->DrawIndexed((size_-1)*(size_-1)*2*3, 0, 0);
  }  
}

float Terrain::GetMinHeight() const {
  return tile_->GetMinHeight();
}

float Terrain::GetMaxHeight() const {
  return tile_->GetMaxHeight();
}

float Terrain::GetHeightAt(const D3DXVECTOR3 &pos) const {
  return tile_->GetHeightAt(pos);
}