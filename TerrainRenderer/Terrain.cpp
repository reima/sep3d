#include <vector>
#include "Terrain.h"
#include "Tile.h"
#include "SDKmesh.h"

// Makro, um die Indexberechnungen für das "flachgeklopfte" 2D-Array von
// Vertices zu vereinfachen
#define I(x,y) ((y)*size_+(x))

Terrain::Terrain(int n, float roughness, int num_lod, float scale, bool water)
    : size_((1 << n) + 1),
      device_(NULL),
      vertex_layout_(NULL),
      vertex_buffer_(NULL),
      index_buffer_(NULL),
      tile_scale_ev_(NULL),
      tile_translate_ev_(NULL),
      tile_lod_ev_(NULL),
      tile_heightmap_ev_(NULL),
      terrain_size_ev_(NULL),
      technique_(NULL),
      indices_(NULL),
      mesh_(NULL),
      mesh_vertex_layout_(NULL),
      mesh_texture_ev_(NULL),
      mesh_texture_srv_(NULL),
      mesh_pass_(NULL),
      tree_buffer_(NULL) {
  tile_ = new Tile(this, n, roughness, num_lod, scale, water);
  InitMeshes();

}

Terrain::~Terrain(void) {
  SAFE_DELETE(indices_);
  SAFE_DELETE(tile_);
  SAFE_DELETE(mesh_);
  ReleaseBuffers();
}

void Terrain::InitMeshes(void) {
  mesh_ = new CDXUTSDKMesh();
  mesh_->Create(DXUTGetD3D10Device(), L"Meshes\\AshTree.sdkmesh");
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

  // Texturen laden
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(device_,
      L"Meshes\\AshTree.png", NULL, NULL, &mesh_texture_srv_, NULL));

  V_RETURN(InitTrees());

  return S_OK;
}

HRESULT Terrain::InitTrees(void) {
  assert(tile_ != NULL);
  assert(device_ != NULL);
  HRESULT hr;
  const int dist = 8;

  std::vector<D3DXMATRIX> tree_transforms;
  srand(42);

  for (int ix = 0; ix < size_; ix += dist) {
    for (int iy = 0; iy < size_; iy += dist) {
      float scaleY = 1 + (rand() / (RAND_MAX * 0.5f) - 1.0f) * 0.3f;
      float scaleXZ = 1 + (rand() / (RAND_MAX * 0.5f) - 1.0f) * 0.1f;
      float rot = (rand() / RAND_MAX) * D3DX_PI;

      D3DXVECTOR3 normal = tile_->vertex_normals_[I(ix,iy)];
      D3DXVECTOR3 base = tile_->GetVectorFromIndex(I(ix,iy));

      // Keine Bäume bei zu großer Steigung
      if (normal.x > normal.y || normal.z > normal.y ) continue;

      base.x += (float)dist/size_ * (rand() / (RAND_MAX * 0.5f) - 1.0f) * 4;
      base.z += (float)dist/size_ * (rand() / (RAND_MAX * 0.5f) - 1.0f) * 4;
      base.y = tile_->GetHeightAt(base);

      // Keine Bäume im Wasser
      if (base.y == 0.0f) continue;

      base.y += 0.5f * scaleY;

      D3DXMATRIX transform;
      D3DXMATRIX temp;
      D3DXMatrixRotationY(&transform, rot);
      D3DXMatrixMultiply(&transform, &transform, D3DXMatrixScaling(&temp, scaleXZ, scaleY, scaleXZ));
      D3DXMatrixMultiply(&transform, &transform, D3DXMatrixTranslation(&temp, base.x, base.y, base.z));

      tree_transforms.push_back(transform);
    }
  }

  num_trees_ = tree_transforms.size();

  D3D10_BUFFER_DESC buffer_desc;
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(D3DXMATRIX) * num_trees_;
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA init_data;
  if (num_trees_ > 0) {
    init_data.pSysMem = &tree_transforms[0];
  } else {
    init_data.pSysMem = NULL;
  }
  init_data.SysMemPitch = 0;
  init_data.SysMemSlicePitch = 0;
  V_RETURN(device_->CreateBuffer(&buffer_desc, &init_data, &tree_buffer_));

  return S_OK;
}

void Terrain::ReleaseBuffers(void) {
  SAFE_RELEASE(vertex_buffer_);
  SAFE_RELEASE(index_buffer_);
  SAFE_RELEASE(vertex_layout_);
  SAFE_RELEASE(mesh_vertex_layout_);
  SAFE_RELEASE(mesh_texture_srv_);
  SAFE_RELEASE(tree_buffer_);
}

void Terrain::GetShaderHandles(ID3D10Effect *effect) {
  assert(device_ != NULL);

  // Terrain Vertex Layout
  const D3D10_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };
  UINT num_elements = sizeof(layout) / sizeof(layout[0]);
  D3D10_PASS_DESC pass_desc;
  effect->GetTechniqueByName("PhongShading")->GetPassByIndex(0)->GetDesc(&pass_desc);
  device_->CreateInputLayout(layout, num_elements, pass_desc.pIAInputSignature,
                             pass_desc.IAInputSignatureSize, &vertex_layout_);

  // Mesh Vertex Layout
  const D3D10_INPUT_ELEMENT_DESC mesh_layout[] = {
    { "POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE" , 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "mTransform", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
    { "mTransform", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
    { "mTransform", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
    { "mTransform", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D10_INPUT_PER_INSTANCE_DATA, 1 },
  };


  num_elements = sizeof(mesh_layout) / sizeof(mesh_layout[0]);
  mesh_pass_ = effect->GetTechniqueByName("Trees")->GetPassByIndex(0);
  mesh_pass_->GetDesc(&pass_desc);
  device_->CreateInputLayout(mesh_layout, num_elements, pass_desc.pIAInputSignature,
                             pass_desc.IAInputSignatureSize, &mesh_vertex_layout_);

  tile_scale_ev_ = effect->GetVariableByName("g_fTileScale")->AsScalar();
  tile_translate_ev_ =
      effect->GetVariableByName("g_vTileTranslate")->AsVector();
  tile_lod_ev_ =
      effect->GetVariableByName("g_uiTileLOD")->AsScalar();
  tile_heightmap_ev_ =
      effect->GetVariableByName("g_tTerrain")->AsShaderResource();
  terrain_size_ev_ =
      effect->GetVariableByName("g_uiTerrainSize")->AsScalar();
  terrain_size_ev_->SetInt(size_);
  mesh_texture_ev_ =
      effect->GetVariableByName("g_tMesh")->AsShaderResource();

}

void Terrain::GetBoundingBox(D3DXVECTOR3 *out, D3DXVECTOR3 *mid) const {
  tile_->GetBoundingBox(out, mid);
}

void Terrain::Draw(ID3D10EffectTechnique *technique, LODSelector *lod_selector,
                   const CBaseCamera *camera) {
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
  tile_->Draw(lod_selector, camera);
  DrawMesh();
  technique_ = NULL;
}

void Terrain::DrawMesh(void) {
  assert(mesh_ != NULL);
  assert(mesh_->IsLoaded());
  assert(mesh_vertex_layout_ != NULL);
  assert(tree_buffer_ != NULL);
  assert(device_ != NULL);

  UINT stride[2] = {
    mesh_->GetVertexStride(0, 0),
    sizeof(D3DXMATRIX)
  };
  UINT offset[2] = { 0, 0 };

  ID3D10Buffer *pVB[2] = {
    mesh_->GetVB10(0, 0),
    tree_buffer_
  };

  device_->IASetVertexBuffers(0, 2, pVB, stride, offset);
  device_->IASetIndexBuffer(mesh_->GetIB10(0), mesh_->GetIBFormat10(0), 0);
  device_->IASetInputLayout(mesh_vertex_layout_);

  SDKMESH_SUBSET *mesh_subset = NULL;
  for (UINT subset = 0; subset < mesh_->GetNumSubsets(0); ++subset) {
    mesh_subset = mesh_->GetSubset(0, subset);
    device_->IASetPrimitiveTopology(
        mesh_->GetPrimitiveType10(
            (SDKMESH_PRIMITIVE_TYPE)mesh_subset->PrimitiveType));

    mesh_texture_ev_->SetResource(mesh_texture_srv_);
    mesh_pass_->Apply(0);

    device_->DrawIndexedInstanced((UINT)mesh_subset->IndexCount,
                                  num_trees_,
                                  0,
                                  (UINT)mesh_subset->VertexStart,
                                  0);
  }
}

void Terrain::DrawTile(float scale, D3DXVECTOR2 &translate, UINT lod,
                       ID3D10ShaderResourceView *srv) {
  assert(tile_scale_ev_ != NULL);
  assert(tile_translate_ev_ != NULL);
  assert(tile_lod_ev_ != NULL);
  assert(tile_heightmap_ev_ != NULL);
  assert(device_ != NULL);
  assert(technique_ != NULL);
  tile_scale_ev_->SetFloat(scale);
  tile_translate_ev_->SetFloatVector(translate);
  tile_lod_ev_->SetInt(lod);
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