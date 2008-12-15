#include <ctime>
#include <cmath>
#include <limits>
#include "Tile.h"
#include "LODSelector.h"
#include "Terrain.h"

#include <D3DX10Math.h>

// Die Makros min und max aus windef.h vertragen sich nicht mit std::min,
// std::max, std::numeric_limits<*>::min, std::numeric_limits<*>::max.
#undef min
#undef max

// Makro, um die Indexberechnungen für das "flachgeklopfte" 2D-Array von
// Vertices zu vereinfachen
#define I(x,y) (static_cast<unsigned int>(y)*size_+static_cast<unsigned int>(x))

namespace {

/**
 * Generiert zufällige Fließkommazahl zwischen -1 und 1
 */
inline float randf(void) {
  return rand() / (RAND_MAX * 0.5f) - 1.0f;
}

}

Tile::Tile(Terrain *terrain, int n, float roughness, int num_lod, float scale)
    : lod_(0),
      size_((1 << n) + 1),
      num_lod_(num_lod),
      vertex_normals_(NULL),
      terrain_(terrain),
      parent_(NULL),
      min_height_(0),
      max_height_(0),
      scale_(scale),
      translation_(D3DXVECTOR2(-.5f*scale_, -.5f*scale)),
      height_map_(NULL),
      shader_resource_view_(NULL) {
  heights_ = new float[size_*size_];
  Init(roughness);
  InitChildren(roughness, NULL, NULL);
  CalculateHeights();
}

Tile::Tile(Tile *parent, Tile::Direction direction,
           float roughness, Tile *north, Tile *west)
    : lod_(parent->lod_ + 1),
      size_(parent->size_),
      num_lod_(parent->num_lod_ - 1),
      vertex_normals_(NULL),
      direction_(direction),
      terrain_(parent->terrain_),
      parent_(parent),
      min_height_(0),
      max_height_(0),
      scale_(parent->scale_*0.5f),
      translation_(parent->translation_),
      height_map_(NULL),
      shader_resource_view_(NULL){
  switch (direction) {
    case NW: translation_ += D3DXVECTOR2(     0,      0); break;
    case NE: translation_ += D3DXVECTOR2(scale_,      0); break;
    case SE: translation_ += D3DXVECTOR2(scale_, scale_); break;
    case SW: translation_ += D3DXVECTOR2(     0, scale_); break;
  }
  heights_ = new float[size_*size_];
  InitFromParent();
  Refine(2, roughness);
  FixEdges(north, west);
  InitChildren(roughness, north, west);
}

Tile::~Tile(void) {
  SAFE_DELETE_ARRAY(heights_);
  SAFE_DELETE_ARRAY(vertex_normals_);
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      delete children_[dir];
    }
  }
  ReleaseBuffers();
}

void Tile::Init(float roughness) {
  // Zufallsgenerator initialisieren
  srand(static_cast<unsigned int>(time(0)));

  // Ecken mit Zufallshöhenwerten initialisieren
  int block_size = size_ - 1;
  heights_[I(0, 0)] = randf();
  heights_[I(0, block_size)]= randf();
  heights_[I(block_size, 0)] = randf();
  heights_[I(block_size, block_size)] = randf();

  // Verfeinerungsschritte durchführen bis sämtliche Werte berechnet sind
  while (block_size > 1) {
    Refine(block_size, roughness);
    block_size = block_size / 2;
  }
}

void Tile::InitFromParent(void) {
  int x1 = 0, y1 = 0;
  switch (direction_) {
    case NW: x1 = 0;         y1 = 0;         break;
    case NE: x1 = size_ / 2; y1 = 0;         break;
    case SW: x1 = 0;         y1 = size_ / 2; break;
    case SE: x1 = size_ / 2; y1 = size_ / 2; break;
  }

  // y-Werte aus dem entsprechenden Quadranten des Eltern-Tiles übernehmen
  for (int y = 0; y < size_; y += 2) {
    for (int x = 0; x < size_; x += 2) {
      heights_[I(x,y)] = parent_->heights_[I(x/2+x1,y/2+y1)];
    }
  }
}

void Tile::Refine(int block_size, float roughness) {
  int block_size_h = block_size/2;
  float offset_factor = (0.2f * scale_) * roughness * block_size / (size_ - 1);

  for (int y = block_size_h; y < size_; y += block_size) {
    for (int x = block_size_h; x < size_; x += block_size) {
      // Lookup der umliegenden Höhenwerte (-); o ist Position (x, y)
      // -   -
      //   o
      // -   -
      float nw = heights_[I(x - block_size_h, y - block_size_h)];
      float ne = heights_[I(x + block_size_h, y - block_size_h)];
      float sw = heights_[I(x - block_size_h, y + block_size_h)];
      float se = heights_[I(x + block_size_h, y + block_size_h)];

      // Berechnung der neuen Höhenwerte (+)
      // - + -
      // + +
      // -   -
      float center = (nw + ne + sw + se) / 4 + offset_factor * randf();
      heights_[I(x, y)] = center;

      float n = nw + ne + center;
      if (y > block_size_h) {
        n += heights_[I(x, y - block_size)];
        n /= 4;
      } else {
        n /= 3;
      }
      heights_[I(x, y - block_size_h)] = n + offset_factor * randf();

      float w = nw + sw + center;
      if (x > block_size_h) {
        w += heights_[I(x - block_size, y)];
        w /= 4;
      } else {
        w /= 3;
      }
      heights_[I(x - block_size_h, y)] = w + offset_factor * randf();

      // Edge cases: Berechnung neuer Höhenwerte am rechten bzw. unteren Rand
      // -   -
      //     +
      // - + -
      if (x == size_ - 1 - block_size_h) {
        heights_[I(x + block_size_h, y)] =
            (ne + se + center) / 3 + offset_factor * randf();
      }
      if (y == size_ - 1 - block_size_h) {
        heights_[I(x, y + block_size_h)] =
            (sw + se + center) / 3 + offset_factor * randf();
      }
    }
  }
}

void Tile::InitChildren(float roughness, Tile *north, Tile *west) {
  if (num_lod_ <= 0) return;

  // Holen benachbarter Kind-Tiles:
  //     +---+---+
  //     |NNW|NNE|
  // +---+---+---+
  // |NWW|       |
  // +---+ this  +
  // |SWW|       |
  // +---+---+---+
  Tile *child_NNW, *child_NNE, *child_NWW, *child_SWW;
  if (north) {
    child_NNW = north->children_[SW];
    child_NNE = north->children_[SE];
  } else {
    child_NNW = child_NNE = NULL;
  }
  if (west) {
    child_NWW = west->children_[NE];
    child_SWW = west->children_[SE];
  } else {
    child_NWW = child_SWW = NULL;
  }

  // Erzeugen der Kind-Tiles mit Übergabe der Nachbarschaftsinformationen
  //roughness /= 2; // Halbiere Rauheits-Faktor, um die geänderten
                  // Größenverhältnisse zu transportieren
  children_[NW] = new Tile(this, NW, roughness, child_NNW, child_NWW);
  children_[NE] = new Tile(this, NE, roughness, child_NNE, children_[NW]);
  children_[SW] = new Tile(this, SW, roughness, children_[NW], child_SWW);
  children_[SE] = new Tile(this, SE, roughness, children_[NE], children_[SW]);
}

void Tile::FixEdges(Tile *north, Tile *west) {
  if (north) {
    for (int x = 1; x < size_; x += 2) {
      heights_[I(x,0)] = north->heights_[I(x,size_-1)];
    }
  }
  if (west) {
    for (int y = 1; y < size_; y += 2) {
      heights_[I(0,y)] = west->heights_[I(size_-1,y)];
    }
  }
}

void Tile::CalculateHeights() {
  assert(heights_ != NULL);
  float min = std::numeric_limits<float>::max();
  float max = std::numeric_limits<float>::min();
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->CalculateHeights();
      min = std::min(min, children_[dir]->min_height_);
      max = std::max(max, children_[dir]->max_height_);
    }
  } else {
    const int res = GetResolution();
    for (int i = 0; i < res; ++i) {
      min = std::min(min, heights_[i]);
      max = std::max(max, heights_[i]);
    }
  }

  min_height_ = min;
  max_height_ = max;
}


float Tile::GetMinHeight(void) const {
  return min_height_;
}

float Tile::GetMaxHeight(void) const {
  return max_height_;
}

float Tile::GetHeightAt(const D3DXVECTOR3 &pos) const {
  D3DXVECTOR2 pos2d = D3DXVECTOR2(pos.x, pos.z);
  if (num_lod_ > 0) {
    D3DXVECTOR2 mid = D3DXVECTOR2(0.5f*scale_, 0.5f*scale_) + translation_;
    if (pos2d.x < mid.x) {
      if (pos2d.y < mid.y) return children_[NW]->GetHeightAt(pos);
      else return children_[SW]->GetHeightAt(pos);
    } else {
      if (pos2d.y < mid.y) return children_[NE]->GetHeightAt(pos);
      else return children_[SE]->GetHeightAt(pos);
    }
  } else {
    // Zwischen den 4 nähesten Vertices interpolieren
    //
    // NW--+-----NE \
    // |   |   /  |  } yfactor
    // +---o-/----+ /
    // |   /      |
    // | / |      |
    // SW--+-----SE
    // \_ _/
    //   V
    // xfactor

    D3DXVECTOR2 texel_coords = (pos2d - translation_) / scale_ *
                               static_cast<float>(size_ - 1);
    // Out of bounds check
    if (texel_coords.x < 0 || texel_coords.x > size_ - 1 ||
        texel_coords.y < 0 || texel_coords.y > size_ - 1) {
      // Strategie 1: Höhe = 0
      //return 0.0f;
      // Strategie 2: Clamping
      texel_coords.x = std::max(std::min(texel_coords.x, (float)size_ - 1), 0.0f);
      texel_coords.y = std::max(std::min(texel_coords.y, (float)size_ - 1), 0.0f);
    }

    float xfactor = std::modf(texel_coords.x, &texel_coords.x);
    float yfactor = std::modf(texel_coords.y, &texel_coords.y);

    if (xfactor == 0.0f) {
      // NW-SW-Linie      
      float height_nw = std::max(heights_[I(texel_coords.x, texel_coords.y)], 0.0f);
      if (yfactor == 0.0f) return height_nw;
      float height_sw = std::max(heights_[I(texel_coords.x, texel_coords.y + 1)], 0.0f);
      return yfactor*height_sw + (1-yfactor)*height_nw;
    } else if (yfactor == 0.0f) {
      // NW-NE-Linie
      float height_nw = std::max(heights_[I(texel_coords.x, texel_coords.y)], 0.0f);
      float height_ne = std::max(heights_[I(texel_coords.x + 1, texel_coords.y)], 0.0f);
      return xfactor*height_ne + (1-xfactor)*height_nw;
    }

    //
    // Korrekte Interpolation im jeweiligen Dreieck
    //
    //if (xfactor + yfactor <= 1.0f) {
    //  // SW-NE-NW-Dreieck
    //  float height_nw = std::max(heights_[I(texel_coords.x, texel_coords.y)], 0.0f);
    //  float height_sw = std::max(heights_[I(texel_coords.x, texel_coords.y + 1)], 0.0f);
    //  float height_ne = std::max(heights_[I(texel_coords.x + 1, texel_coords.y)], 0.0f);
    //  float height_w = yfactor*height_sw + (1-yfactor)*height_nw;
    //  float height_e = yfactor*height_sw + (1-yfactor)*height_ne;
    //  return (xfactor*height_e + (1-yfactor-xfactor)*height_w)/(1-yfactor);
    //} else {
    //  // SW-SE-NE-Dreieck
    //  float height_sw = std::max(heights_[I(texel_coords.x, texel_coords.y + 1)], 0.0f);
    //  float height_ne = std::max(heights_[I(texel_coords.x + 1, texel_coords.y)], 0.0f);      
    //  float height_se = std::max(heights_[I(texel_coords.x + 1, texel_coords.y + 1)], 0.0f);
    //  float height_w = yfactor*height_sw + (1-yfactor)*height_ne;
    //  float height_e = yfactor*height_se + (1-yfactor)*height_ne;
    //  return ((1-xfactor)*height_w + (yfactor-(1-xfactor))*height_e)/yfactor;
    //}

    //
    // Bilineare Interpolation im Quadrat
    //
    float height_nw = std::max(heights_[I(texel_coords.x, texel_coords.y)], 0.0f);
    float height_sw = std::max(heights_[I(texel_coords.x, texel_coords.y + 1)], 0.0f);
    float height_ne = std::max(heights_[I(texel_coords.x + 1, texel_coords.y)], 0.0f);      
    float height_se = std::max(heights_[I(texel_coords.x + 1, texel_coords.y + 1)], 0.0f);
    float height_w = yfactor*height_sw + (1-yfactor)*height_nw;
    float height_e = yfactor*height_se + (1-yfactor)*height_ne;
    return xfactor*height_e + (1-xfactor)*height_w;
  }
}

HRESULT Tile::CreateBuffers(ID3D10Device *device) {
  assert(heights_ != NULL);
  assert(vertex_normals_ != NULL);
  HRESULT hr;

  // Evtl. bereits vorhandene Buffer freigeben
  ReleaseBuffers();

  const int resolution = GetResolution();
  D3DXVECTOR4 *normals_heights = new D3DXVECTOR4[resolution];
  for (int i = 0; i < resolution; ++i) {
    normals_heights[i] = D3DXVECTOR4(vertex_normals_[i], heights_[i]);
  }

  // Textur anlegen
  D3D10_TEXTURE2D_DESC tex2d_desc;
  tex2d_desc.Width = size_;
  tex2d_desc.Height = size_;
  tex2d_desc.MipLevels = 1;
  tex2d_desc.ArraySize = 1;
  tex2d_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  tex2d_desc.SampleDesc.Count = 1;
  tex2d_desc.SampleDesc.Quality = 0;
  tex2d_desc.Usage = D3D10_USAGE_IMMUTABLE;
  tex2d_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  tex2d_desc.CPUAccessFlags = 0;
  tex2d_desc.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = normals_heights;
  init_data.SysMemPitch = sizeof(normals_heights[0]) * size_;
  init_data.SysMemSlicePitch = 0;
  V_RETURN(device->CreateTexture2D(&tex2d_desc, &init_data, &height_map_));

  delete[] normals_heights;

  // Shader Resource View anlegen
  D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
  srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MipLevels = 1;
  srv_desc.Texture2D.MostDetailedMip = 0;
  V_RETURN(device->CreateShaderResourceView(height_map_, &srv_desc,
                                            &shader_resource_view_));

  // Rekursiver Aufruf über alle Kinder
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      V_RETURN(children_[dir]->CreateBuffers(device));
    }
  }

  return S_OK;
}

void Tile::ReleaseBuffers(void) {
  SAFE_RELEASE(height_map_);
  SAFE_RELEASE(shader_resource_view_);
}

void Tile::FreeMemory(void) {
  SAFE_DELETE_ARRAY(heights_);
  SAFE_DELETE_ARRAY(vertex_normals_);
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->FreeMemory();
    }
  }
}

void Tile::Draw(LODSelector *lod_selector, const CBaseCamera *camera) {
  assert(terrain_ != NULL);
  assert(shader_resource_view_ != NULL);
  if (lod_selector->IsLODSufficient(this, camera) || num_lod_ == 0) {
    terrain_->DrawTile(scale_, translation_, lod_, shader_resource_view_);
  } else {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->Draw(lod_selector, camera);
    }
  }
}

void Tile::CalculateNormals(unsigned int *indices) {
  CalculateNormals0(NULL, NULL, indices);
  NormalizeNormals();
}

D3DXVECTOR3 Tile::GetVectorFromIndex(int index) {
  assert(heights_ != NULL);
  float x = (float)(index % size_) / (size_ - 1) * scale_ + translation_.x;
  float z = (float)(index / size_) / (size_ - 1) * scale_ + translation_.y;
  return D3DXVECTOR3(x, heights_[index], z);
}

void Tile::CalculateNormals0(Tile *north, Tile *west, unsigned int *indices) {
  assert(heights_ != NULL);
  SAFE_DELETE_ARRAY(vertex_normals_);
  const int num_vertices = GetResolution();
  vertex_normals_ = new D3DXVECTOR3[num_vertices];
  // Normalen auf 0 initialisieren
  for (int i = 0; i < num_vertices; ++i) {
    vertex_normals_[i] = D3DXVECTOR3(0.f, 0.f, 0.f);
  }
  // Normalen-Zwischenwerte ggf. von Nachbarn holen
  if (north) {
    for (int x = 0; x < size_; ++x) {
      vertex_normals_[I(x, 0)] = north->vertex_normals_[I(x, size_ - 1)];
    }
  }
  if (west) {
    for (int y = 0; y < size_; ++y) {
      vertex_normals_[I(0, y)] = west->vertex_normals_[I(size_ - 1, y)];
    }
  }
  // Face-Normalen berechnen und auf die Normalen der beteiligten Vertices
  // aufaddieren
  int num_triangles = (size_-1)*(size_-1) * 2;
  for (int i = 0; i < num_triangles; ++i) {
    D3DXVECTOR3 v1 = GetVectorFromIndex(indices[3*i]);
    D3DXVECTOR3 v2 = GetVectorFromIndex(indices[3*i+1]);
    D3DXVECTOR3 v3 = GetVectorFromIndex(indices[3*i+2]);
    D3DXVECTOR3 e1 = v2 - v1;
    D3DXVECTOR3 e2 = v3 - v1;
    D3DXVECTOR3 face_normal;
    D3DXVec3Cross(&face_normal, &e1, &e2);
    D3DXVec3Normalize(&face_normal, &face_normal);
    vertex_normals_[indices[3*i]] += face_normal;
    vertex_normals_[indices[3*i+1]] += face_normal;
    vertex_normals_[indices[3*i+2]] += face_normal;
  }
  // Normalen ggf. an Nachbarn zurückgeben
  if (north) {
    for (int x = 0; x < size_; ++x) {
      north->vertex_normals_[I(x, size_ - 1)] = vertex_normals_[I(x, 0)];
    }
  }
  if (west) {
    for (int y = 0; y < size_; ++y) {
      west->vertex_normals_[I(size_ - 1, y)] = vertex_normals_[I(0, y)];
    }
  }

  if (num_lod_ <= 0) return;

  // Holen benachbarter Kind-Tiles:
  //     +---+---+
  //     |NNW|NNE|
  // +---+---+---+
  // |NWW|       |
  // +---+ this  +
  // |SWW|       |
  // +---+---+---+
  Tile *child_NNW, *child_NNE, *child_NWW, *child_SWW;
  if (north) {
    child_NNW = north->children_[SW];
    child_NNE = north->children_[SE];
  } else {
    child_NNW = child_NNE = NULL;
  }
  if (west) {
    child_NWW = west->children_[NE];
    child_SWW = west->children_[SE];
  } else {
    child_NWW = child_SWW = NULL;
  }
  children_[NW]->CalculateNormals0(child_NNW, child_NWW, indices);
  children_[NE]->CalculateNormals0(child_NNE, children_[NW], indices);
  children_[SW]->CalculateNormals0(children_[NW], child_SWW, indices);
  children_[SE]->CalculateNormals0(children_[NE], children_[SW], indices);
}

void Tile::NormalizeNormals(void) {
  assert(vertex_normals_ != NULL);
  const int num_vertices = GetResolution();
  for (int i = 0; i < num_vertices; ++i) {
    D3DXVec3Normalize(&vertex_normals_[i], &vertex_normals_[i]);
  }
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->NormalizeNormals();
    }
  }
}

void Tile::GetBoundingBox(D3DXVECTOR3 *box, D3DXVECTOR3 *mid) const {
  box[0] = D3DXVECTOR3(0, min_height_, 0);
  box[1] = D3DXVECTOR3(0, min_height_, 1);
  box[2] = D3DXVECTOR3(1, min_height_, 0);
  box[3] = D3DXVECTOR3(1, min_height_, 1);
  box[4] = D3DXVECTOR3(0, max_height_, 0);
  box[5] = D3DXVECTOR3(0, max_height_, 1);
  box[6] = D3DXVECTOR3(1, max_height_, 0);
  box[7] = D3DXVECTOR3(1, max_height_, 1);
  for (int i = 0; i < 8; ++i) {
    box[i] *= scale_;
    box[i].x += translation_.x;
    box[i].z += translation_.y;
  }
  if (mid != NULL) *mid = 0.5f * (box[0] + box[7]);
}

float Tile::GetWorldError(void) const {
  return scale_ / (size_ - 1);
}