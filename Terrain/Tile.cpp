#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include "IOTools.h"
#include "Tile.h"

// Die Makros min und max aus windef.h vertragen sich nicht mit std::min,
// std::max, std::numeric_limits<*>::min, std::numeric_limits<*>::max.
#undef min
#undef max

#define M(x,y) (y)*size_+(x)

namespace {

/**
 * Generiert zufällige Fließkommazahl zwischen -1 und 1
 */
inline float randf() {
  return rand() / (RAND_MAX * 0.5f) - 1.0f;
}

}

Tile::Tile(int n, float roughness, int num_lod)
    : lod_(0),
      size_((1 << n) + 1),
      num_lod_(num_lod),
      index_buffer_(NULL),
      parent_(NULL) {
  vertices_ = new Vector[size_*size_];
  Init(roughness);
  InitChildren(roughness, NULL, NULL);
}

Tile::Tile(Tile *parent, Tile::Direction direction, float roughness,
           Tile *north, Tile *west)
    : lod_(parent->lod_ + 1),
      size_(parent->size_),
      num_lod_(parent->num_lod_ - 1),
      index_buffer_(NULL),
      direction_(direction),
      parent_(parent) {
  vertices_ = new Vector[size_*size_];
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
  delete[] index_buffer_;
  delete[] vertices_;
}

void Tile::Init(float roughness) {
  srand(static_cast<unsigned int>(time(0)));
  int block_size = size_ - 1;

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
  vertices_[M(0, 0)].y = randf();
  vertices_[M(0, block_size)].y = randf();
  vertices_[M(block_size, 0)].y = randf();
  vertices_[M(block_size, block_size)].y = randf();

  // Verfeinerungsschritte durchführen bis sämtliche Werte berechnet sind
  while (block_size > 1) {
    Refine(block_size, roughness);
    block_size = block_size / 2;
  }
}

void Tile::InitFromParent(void) {
  int x1, y1;
  switch (direction_) {
    case NW: x1 = 0;         y1 = 0;         break;
    case NE: x1 = size_ / 2; y1 = 0;         break;
    case SW: x1 = 0;         y1 = size_ / 2; break;
    case SE: x1 = size_ / 2; y1 = size_ / 2; break;
  }

  // y-Werte aus dem entsprechenden Quadranten des Eltern-Tiles übernehmen
  for (int y = 0; y < size_; y += 2) {
    for (int x = 0; x < size_; x += 2) {
      vertices_[M(x,y)].y = parent_->vertices_[M(x/2+x1,y/2+y1)].y;
    }
  }

  // x- und z-Koordinaten berechnen
  Vector &v_min = parent_->vertices_[M(x1, y1)];
  Vector &v_max = parent_->vertices_[M(x1 + size_ / 2, y1 + size_ / 2)];
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
      float nw = vertices_[M(x - block_size_h, y - block_size_h)].y;
      float ne = vertices_[M(x + block_size_h, y - block_size_h)].y;
      float sw = vertices_[M(x - block_size_h, y + block_size_h)].y;
      float se = vertices_[M(x + block_size_h, y + block_size_h)].y;
      
      // Berechnung der neuen Höhenwerte (+)
      // - + -
      // + +
      // -   -
      float center = (nw + ne + sw + se) / 4 + offset_factor * randf();
      vertices_[M(x, y)].y = center;
      
      float n = nw + ne + center;
      if (y > block_size_h) {
        n += vertices_[M(x, y - block_size)].y;
        n /= 4;
      } else {
        n /= 3;
      }
      vertices_[M(x, y - block_size_h)].y = n + offset_factor * randf();

      float w = nw + sw + center;
      if (x > block_size_h) {
        w += vertices_[M(x - block_size, y)].y;
        w /= 4;
      } else {
        w /= 3;
      }
      vertices_[M(x - block_size_h, y)].y = w + offset_factor * randf();

      // Edge cases: Berechnung neuer Höhenwerte am rechten bzw. unteren Rand
      // -   -
      //     +
      // - + - 
      if (x == size_ - 1 - block_size_h) {
        vertices_[M(x + block_size_h, y)].y =
            (ne + se + center) / 3 + offset_factor * randf();
      }
      if (y == size_ - 1 - block_size_h) {
        vertices_[M(x, y + block_size_h)].y =
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
  children_[NW] = new Tile(this, NW, roughness, child_NNW, child_NWW);
  children_[NE] = new Tile(this, NE, roughness, child_NNE, children_[NW]);
  children_[SW] = new Tile(this, SW, roughness, children_[NW], child_SWW);
  children_[SE] = new Tile(this, SE, roughness, children_[NE], children_[SW]);
}

void Tile::FixEdges(Tile *north, Tile *west) {
  if (north) {
    for (int x = 1; x < size_; x += 2) {
      vertices_[M(x,0)].y = north->vertices_[M(x,size_-1)].y;
    }
  }
  if (west) {
    for (int y = 1; y < size_; y += 2) {
      vertices_[M(0,y)].y = west->vertices_[M(size_-1,y)].y;
    }
  }
}

float Tile::GetMinHeight() const {
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

float Tile::GetMaxHeight() const {
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

void Tile::SaveImages(const std::wstring &filename) const {
  float min = GetMinHeight();
  float max = GetMaxHeight();
  std::wstring basename(filename);
  basename.erase(basename.rfind('.'), basename.size());
  std::wstring extension(filename, filename.rfind('.'));
  int image_size = size_;
  for (int lod = 0; lod <= num_lod_; ++lod) {
    unsigned char *image_data = new unsigned char[3 * image_size * image_size];
    SaveImageTilesForLOD(image_data, image_size, 0, 0, lod);
    std::wstringstream ss;
    ss << basename << lod << extension;
    IOTools::SaveImage(ss.str().c_str(), image_size, image_size, image_data, true);
    image_size = 2 * image_size - 1;
    delete[] image_data;
 }
}

void Tile::SaveImageTilesForLOD(unsigned char *image_data, int image_size,
                                int x_off, int y_off, int lod) const {
  if (lod > 0) {
    // Noch nicht tief genug abgestiegen, Tile mit höherem LOD gesucht
    children_[NW]->SaveImageTilesForLOD(image_data, image_size,
                                        x_off, y_off, lod - 1);
    children_[NE]->SaveImageTilesForLOD(image_data, image_size,
                                        x_off + (size_ - 1) * (1 << (lod - 1)), y_off, lod - 1);
    children_[SW]->SaveImageTilesForLOD(image_data, image_size,
                                        x_off, y_off + (size_ - 1) * (1 << (lod - 1)), lod - 1);
    children_[SE]->SaveImageTilesForLOD(image_data, image_size,
                                        x_off + (size_ - 1) * (1 << (lod - 1)),
                                        y_off + (size_ - 1) * (1 << (lod - 1)), lod - 1);
    // Hier gibt es sonst nichts zu tun...
    return;
  }

  static const int num_spots = 8;
  static const float spots[num_spots] = {
    -1.0f,      // Tiefes Wasser
    -0.25f,     // Seichtes Wasser
     0.0f,      // Küste
     0.0625f,   // Strand
     0.125f,    // Gras
     0.375f,    // Wald
     0.75f,     // Gestein
     1.0f       // Schnee
  };
  static const unsigned char colors[num_spots][3] = {
    { 160, 0, 0 },      // Tiefes Wasser
    { 255, 64, 0 },     // Seichtes Wasser
    { 255, 128, 0 },    // Küste
    { 64, 240, 240 },   // Strand
    { 0, 160, 32 },     // Gras
    { 0, 96, 0 },       // Wald
    { 128, 128, 128 },  // Gestein
    { 255, 255, 255 }   // Schnee
  };
  int i = 0;
  int image_index = 3 * (y_off * image_size + x_off);
  for (int y = 0; y < size_; ++y, image_index += 3 * (image_size - size_)) {
    for (int x = 0; x < size_; ++x, ++i, image_index += 3) {
      float height = vertices_[i].y;      
      if (height < spots[0]) {
        // Wert liegt unterhalb der Bereiche
        image_data[image_index]   = colors[0][0];
        image_data[image_index+1] = colors[0][1];
        image_data[image_index+2] = colors[0][2];
      } else if (height > spots[num_spots - 1]) {
        // Wert liegt oberhalb der Bereiche
        image_data[image_index]   = colors[num_spots - 1][0];
        image_data[image_index+1] = colors[num_spots - 1][1];
        image_data[image_index+2] = colors[num_spots - 1][2];
      } else {
        // Wert liegt innerhalb eines Bereichs
        for (int j = 0; j < num_spots - 1; ++j) {
          if (spots[j] <= height && height <= spots[j+1]) {
            float interpolated_value = (height - spots[j]) / (spots[j+1] - spots[j]);
            // Interpolation der beiden angrenzenden Farben des Bereichs
            for (int c = 0; c < 3; ++c) {
              image_data[image_index+c] =
                  static_cast<unsigned char>(
                      (1 - interpolated_value) * colors[j][c] +
                      interpolated_value * colors[j+1][c]);
            }
            break;
          } // if
        } // for
      } // else
    } // for (int x = 0; ...
  } // for (int y = 0; ...
}

void Tile::InitIndexBuffer(void) {
  if (index_buffer_ == NULL) {
    index_buffer_ = new unsigned int[6*(size_-1)*(size_-1)];
  }    
}

void Tile::TriangulateLines(void)  {
  InitIndexBuffer();
  // Dreieck 1 links oben, Dreieck 2 rechts unten
  int i = 0;
  for (int y = 0; y < size_ - 1; y++) {
    for (int x = 0; x < size_ - 1; x++) {
      index_buffer_[i++] = M(x, y);     // 1. Dreieck links oben
      index_buffer_[i++] = M(x+1, y);   // 1. Dreieck rechts oben
      index_buffer_[i++] = M(x, y+1);   // 1. Dreieck links unten
      index_buffer_[i++] = M(x+1, y);   // 2. Dreieck rechts oben
      index_buffer_[i++] = M(x+1, y+1); // 2. Dreieck rechts unten
      index_buffer_[i++] = M(x, y+1);   // 2. Dreieck links unten
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
    index_buffer_[i++] = M(x1, y1);     // 1. Dreieck links oben
    index_buffer_[i++] = M(x1+1, y1);   // 1. Dreieck rechts oben
    index_buffer_[i++] = M(x1, y1+1);   // 1. Dreieck links unten
    index_buffer_[i++] = M(x1+1, y1);   // 2. Dreieck rechts oben
    index_buffer_[i++] = M(x1+1, y1+1); // 2. Dreieck rechts unten
    index_buffer_[i++] = M(x1, y1+1);   // 2. Dreieck links unten
  } else {
    int x12 = (x1+x2)/2;
    int y12 = (y1+y2)/2;
 
    TriangulateZOrder0(x1, y1, x12, y12, i);
    TriangulateZOrder0(x12, y1, x2, y12, i);
    TriangulateZOrder0(x1, y12, x12, y2, i);
    TriangulateZOrder0(x12, y12, x2, y2, i);
  }
}

void Tile::SaveObj(const std::wstring &filename) const {
  std::wstring basename(filename);
  basename.erase(basename.rfind('.'), basename.size());
  std::wstring extension(filename, filename.rfind('.'));
  SaveObj0(basename, extension);
}

void Tile::SaveObj0(const std::wstring &basename,
                    const std::wstring &extension) const {
  std::wstring filename(basename);
  filename.append(extension);

  std::ofstream ofs(filename.c_str());
  ofs << "# Terrain file" << std::endl;
  // Vertices & Texturkoordinaten
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x) {
      // Vertex
      Vector &v = vertices_[M(x,y)];
      ofs << "v ";
      ofs << v.x << " ";
      ofs << std::max(0.0f, v.y) << " ";
      ofs << v.z << std::endl;
      // Texturkoordinaten
      ofs << "vt ";
      ofs << ((float)x/size_) << " ";
      ofs << ((float)y/size_) << std::endl;
    }
  }
  // Faces (Dreiecke)
  if (index_buffer_) {
    const int num_triangles = 2 * (size_ - 1) * (size_ - 1);
    for (int i = 0; i < num_triangles; ++i) {
      ofs << "f ";
      ofs << (index_buffer_[3*i]+1) << "/";
      ofs << (index_buffer_[3*i]+1) << " ";
      ofs << (index_buffer_[3*i+1]+1) << "/";
      ofs << (index_buffer_[3*i+1]+1) << " ";
      ofs << (index_buffer_[3*i+2]+1) << "/";
      ofs << (index_buffer_[3*i+2]+1) << std::endl;
    }
  }

  if (num_lod_ > 0) {
    std::wstring filename;
    filename = basename;
    filename.append(L"0");
    children_[NW]->SaveObj0(filename, extension);
    filename = basename;
    filename.append(L"1");
    children_[NE]->SaveObj0(filename, extension);
    filename = basename;
    filename.append(L"2");
    children_[SW]->SaveObj0(filename, extension);
    filename = basename;
    filename.append(L"3");
    children_[SE]->SaveObj0(filename, extension);
  }
}
