#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include "Tile.h"
#include "LODSelector.h"

// Die Makros min und max aus windef.h vertragen sich nicht mit std::min,
// std::max, std::numeric_limits<*>::min, std::numeric_limits<*>::max.
#undef min
#undef max

// Makro, um die Indexberechnungen für das "flachgeklopfte" 2D-Array von
// Vertices zu vereinfachen
#define I(x,y) ((y)*size_+(x))

namespace {

/**
 * Generiert zufällige Fließkommazahl zwischen -1 und 1
 */
inline float randf(void) {
  return rand() / (RAND_MAX * 0.5f) - 1.0f;
}

}

Tile::Tile(int n, float roughness, int num_lod)
    : lod_(0),
      size_((1 << n) + 1),
      num_lod_(num_lod),
      indices_(NULL),
      parent_(NULL),
      vertex_buffer_(NULL),
      normal_buffer_(NULL),
      index_buffer_(NULL) {
  vertices_ = new D3DXVECTOR3[size_*size_];
  Init(roughness);
  InitChildren(roughness, NULL, NULL);
}

Tile::Tile(Tile *parent, Tile::Direction direction, float roughness,
           Tile *north, Tile *west)
    : lod_(parent->lod_ + 1),
      size_(parent->size_),
      num_lod_(parent->num_lod_ - 1),
      indices_(NULL),
      direction_(direction),
      parent_(parent),
      vertex_buffer_(NULL),
      normal_buffer_(NULL),
      index_buffer_(NULL) {
  vertices_ = new D3DXVECTOR3[size_*size_];
  InitFromParent();
  Refine(2, roughness);
  FixEdges(north, west);
  InitChildren(roughness, north, west);
}

Tile::~Tile(void) {
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      delete children_[dir];
    }
  }
  SAFE_DELETE_ARRAY(indices_);
  SAFE_DELETE_ARRAY(vertices_);
  ReleaseBuffers();
}

void Tile::Init(float roughness) {
  // Zufallsgenerator initialisieren
  srand(static_cast<unsigned int>(time(0)));

  // x- und z-Koordinaten berechnen.
  // Das Wurzel-Tile ersteckt sich entlang der x- und z-Achse immer im Bereich
  // [-2.5, 2.5]
  int i = 0;
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x, ++i) {
      vertices_[i].x = x * 5.0f / (size_ - 1) - 2.5f;
      vertices_[i].z = y * 5.0f / (size_ - 1) - 2.5f;
    }
  }

  // Ecken mit Zufallshöhenwerten initialisieren
  int block_size = size_ - 1;
  vertices_[I(0, 0)].y = randf();
  vertices_[I(0, block_size)].y = randf();
  vertices_[I(block_size, 0)].y = randf();
  vertices_[I(block_size, block_size)].y = randf();

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
      vertices_[I(x,y)].y = parent_->vertices_[I(x/2+x1,y/2+y1)].y;
    }
  }

  // x- und z-Koordinaten berechnen
  D3DXVECTOR3 &v_min = parent_->vertices_[I(x1, y1)];
  D3DXVECTOR3 &v_max = parent_->vertices_[I(x1 + size_ / 2, y1 + size_ / 2)];
  int i = 0;
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x, ++i) {
      vertices_[i].x = x * (v_max.x - v_min.x) / (size_ - 1) + v_min.x;
      vertices_[i].z = y * (v_max.z - v_min.z) / (size_ - 1) + v_min.z;
    }
  }
}

void Tile::Refine(int block_size, float roughness) {
  int block_size_h = block_size/2;
  float offset_factor = roughness * block_size / size_;

  for (int y = block_size_h; y < size_; y += block_size) {
    for (int x = block_size_h; x < size_; x += block_size) {
      // Lookup der umliegenden Höhenwerte (-); o ist Position (x, y)
      // -   -
      //   o
      // -   -
      float nw = vertices_[I(x - block_size_h, y - block_size_h)].y;
      float ne = vertices_[I(x + block_size_h, y - block_size_h)].y;
      float sw = vertices_[I(x - block_size_h, y + block_size_h)].y;
      float se = vertices_[I(x + block_size_h, y + block_size_h)].y;
      
      // Berechnung der neuen Höhenwerte (+)
      // - + -
      // + +
      // -   -
      float center = (nw + ne + sw + se) / 4 + offset_factor * randf();
      vertices_[I(x, y)].y = center;
      
      float n = nw + ne + center;
      if (y > block_size_h) {
        n += vertices_[I(x, y - block_size)].y;
        n /= 4;
      } else {
        n /= 3;
      }
      vertices_[I(x, y - block_size_h)].y = n + offset_factor * randf();

      float w = nw + sw + center;
      if (x > block_size_h) {
        w += vertices_[I(x - block_size, y)].y;
        w /= 4;
      } else {
        w /= 3;
      }
      vertices_[I(x - block_size_h, y)].y = w + offset_factor * randf();

      // Edge cases: Berechnung neuer Höhenwerte am rechten bzw. unteren Rand
      // -   -
      //     +
      // - + - 
      if (x == size_ - 1 - block_size_h) {
        vertices_[I(x + block_size_h, y)].y =
            (ne + se + center) / 3 + offset_factor * randf();
      }
      if (y == size_ - 1 - block_size_h) {
        vertices_[I(x, y + block_size_h)].y =
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
  roughness /= 2; // Halbiere Rauheits-Faktor, um die geänderten
                  // Größenverhältnisse zu transportieren
  children_[NW] = new Tile(this, NW, roughness, child_NNW, child_NWW);
  children_[NE] = new Tile(this, NE, roughness, child_NNE, children_[NW]);
  children_[SW] = new Tile(this, SW, roughness, children_[NW], child_SWW);
  children_[SE] = new Tile(this, SE, roughness, children_[NE], children_[SW]);
}

void Tile::FixEdges(Tile *north, Tile *west) {
  if (north) {
    for (int x = 1; x < size_; x += 2) {
      vertices_[I(x,0)].y = north->vertices_[I(x,size_-1)].y;
    }
  }
  if (west) {
    for (int y = 1; y < size_; y += 2) {
      vertices_[I(0,y)].y = west->vertices_[I(size_-1,y)].y;
    }
  }
}

float Tile::GetMinHeight(void) const {
  static float min = std::numeric_limits<float>::max();
  if (min == std::numeric_limits<float>::max()) {
    const int res = GetResolution();
    for (int i = 0; i < res; ++i) {
      min = std::min(min, vertices_[i].y);
    }
    if (num_lod_ > 0) {
      for (int dir = 0; dir < 4; ++dir) {
        min = std::min(min, children_[dir]->GetMinHeight());
      }
    }
  }
  return min;
}

float Tile::GetMaxHeight(void) const {
  static float max = std::numeric_limits<float>::min();
  if (max == std::numeric_limits<float>::min()) {
    const int res = GetResolution();
    for (int i = 1; i < res; ++i) {
      max = std::max(max, vertices_[i].y);
    }
    if (num_lod_ > 0) {
      for (int dir = 0; dir < 4; ++dir) {
        max = std::max(max, children_[dir]->GetMaxHeight());
      }
    }
  }
  return max;
}

void Tile::InitIndexBuffer(void) {
  if (indices_ == NULL) {
    // (size_-1)^2 Blöcke, pro Block 2 Dreiecke, pro Dreieck 3 Indizes
    indices_ = new unsigned int[(size_-1)*(size_-1)*2*3];
  }    
}

void Tile::TriangulateLines(void)  {
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
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->TriangulateLines();
    }
  }
}

void Tile::TriangulateZOrder(void) {
  InitIndexBuffer();
  int i = 0;
  TriangulateZOrder0(0, 0, size_-1, size_-1, i);
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->TriangulateZOrder();
    }
  }
}

void Tile::TriangulateZOrder0(int x1, int y1, int x2, int y2, int &i){
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

void Tile::SaveObjs(const std::wstring &filename) const {
  std::wstring basename(filename);
  basename.erase(basename.rfind('.'), basename.size());
  std::wstring extension(filename, filename.rfind('.'));
  SaveObjs0(basename, extension);
}

void Tile::SaveObjs0(const std::wstring &basename,
                     const std::wstring &extension) const {
  std::wstring filename(basename);
  filename.append(extension);

  std::ofstream ofs(filename.c_str());
  ofs << "# Terrain file" << std::endl;
  // Vertices
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x) {
      D3DXVECTOR3 &v = vertices_[I(x,y)];
      ofs << "v ";
      ofs << v.x << " ";
      // Alle Höhenwerte < 0 sind Wasser, das Terrain darunter ist für die
      // gewünschte Darstellung uninteressant
      ofs << std::max(0.0f, v.y) << " ";
      ofs << v.z << std::endl;
    }
  }
  // Faces (Dreiecke)
  if (indices_) {
    const int num_triangles = 2 * (size_ - 1) * (size_ - 1);
    for (int i = 0; i < num_triangles; ++i) {
      ofs << "f ";
      ofs << (indices_[3*i]+1) << " ";
      ofs << (indices_[3*i+1]+1) << " ";
      ofs << (indices_[3*i+2]+1) << std::endl;
    }
  }

  if (num_lod_ > 0) {
    std::wstring filename;
    filename = basename;
    filename.append(L"0");
    children_[NW]->SaveObjs0(filename, extension);
    filename = basename;
    filename.append(L"1");
    children_[NE]->SaveObjs0(filename, extension);
    filename = basename;
    filename.append(L"2");
    children_[SW]->SaveObjs0(filename, extension);
    filename = basename;
    filename.append(L"3");
    children_[SE]->SaveObjs0(filename, extension);
  }
}

HRESULT Tile::CreateBuffers(ID3D10Device *pd3dDevice) {
  HRESULT hr;

  // Evtl. bereits vorhandene Buffer freigeben
  ReleaseBuffers();

  // Vertex Buffer anlegen
  D3D10_BUFFER_DESC buffer_desc;
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(D3DXVECTOR3) * GetResolution();
  buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;

  D3D10_SUBRESOURCE_DATA init_data;
  init_data.pSysMem = vertices_;
  init_data.SysMemPitch = 0;
  init_data.SysMemSlicePitch = 0;

  V_RETURN(pd3dDevice->CreateBuffer(&buffer_desc,
                                    &init_data,
                                    &vertex_buffer_));

  // Index Buffer anlegen
  buffer_desc.Usage = D3D10_USAGE_DEFAULT;
  buffer_desc.ByteWidth = sizeof(unsigned int) * (size_ - 1)*(size_ - 1)*2*3;
  buffer_desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;

  init_data.pSysMem = indices_;

  V_RETURN(pd3dDevice->CreateBuffer(&buffer_desc,
                                    &init_data,
                                    &index_buffer_));

  // Rekursiver Aufruf über alle Kinder
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      V_RETURN(children_[dir]->CreateBuffers(pd3dDevice));
    }
  }

  return S_OK;
}

void Tile::ReleaseBuffers(void) {
  SAFE_RELEASE(vertex_buffer_);
  SAFE_RELEASE(index_buffer_);
}

void Tile::FreeMemory(void) {
  SAFE_DELETE_ARRAY(vertices_);
  SAFE_DELETE_ARRAY(indices_);
  if (num_lod_ > 0) {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->FreeMemory();
    }
  }
}

void Tile::Draw(ID3D10Device *pd3dDevice, LODSelector *lod_selector,
                const D3DXVECTOR3 *cam_pos) const {
  if (lod_selector->IsLODSufficient(this, cam_pos) || num_lod_ == 0) {
    // Vertex Buffer setzen
    UINT stride = sizeof(D3DXVECTOR3);
    UINT offset = 0;
    pd3dDevice->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);

    // Index Buffer setzen
    pd3dDevice->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);

    // Rendern
    pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dDevice->DrawIndexed((size_-1)*(size_-1)*2*3, 0, 0);
  } else {
    for (int dir = 0; dir < 4; ++dir) {
      children_[dir]->Draw(pd3dDevice, lod_selector, cam_pos);
    }
  }
}
