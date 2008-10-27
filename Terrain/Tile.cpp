#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <limits>
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

Tile::Tile(int lod, float roughness, int num_lod)
    : lod_(lod),
      size_((1 << lod) + 1),
      num_lod_(num_lod),
      index_buffer_(NULL),
      parent_(NULL) {
  height_map_ = new float[size_*size_];
  Init(roughness);
  InitChildren(roughness, NULL, NULL);
}

Tile::Tile(Tile *parent, Tile::Direction direction, float roughness,
           Tile *north, Tile *west)
    : lod_(parent->lod_),
      size_(parent->size_),
      num_lod_(parent->num_lod_ - 1),
      index_buffer_(NULL),
      direction_(direction),
      parent_(parent) {
  height_map_ = new float[size_*size_];
  InitFromParent();
  Refine(2, roughness);
  FixEdges(north, west);
  InitChildren(roughness, north, west);
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
  children_[NW] = new Tile(this, NW, roughness, child_NNW, child_NWW);
  children_[NE] = new Tile(this, NE, roughness, child_NNE, children_[NW]);
  children_[SW] = new Tile(this, SW, roughness, children_[NW], child_SWW);
  children_[SE] = new Tile(this, SE, roughness, children_[NE], children_[SW]);
}

void Tile::InitFromParent(void) {
  int x1, y1;
  switch (direction_) {
    case NW:
      x1 = 0;
      y1 = 0;
      break;
    case NE:
      x1 = size_ / 2;
      y1 = 0;
      break;
    case SW:
      x1 = 0;
      y1 = size_ / 2;
      break;
    case SE:
      x1 = size_ / 2;
      y1 = size_ / 2;
      break;
  }

  for (int y = 0; y < size_; y += 2)
    for (int x = 0; x < size_; x += 2)
      height_map_[M(x,y)] = parent_->height_map_[M(x/2+x1,y/2+y1)];
}

void Tile::FixEdges(Tile *north, Tile *west) {
  if (north) {
    for (int x = 1; x < size_; x += 2) {
      height_map_[M(x,0)] = north->height_map_[M(x,size_-1)];
    }
  }
  if (west) {
    for (int y = 1; y < size_; y += 2) {
      height_map_[M(0,y)] = west->height_map_[M(size_-1,y)];
    }
  }
}

Tile::~Tile(void) {
  if (num_lod_ > 0) {
    delete children_[NW];
    delete children_[NE];
    delete children_[SW];
    delete children_[SE];
  }
  delete[] index_buffer_;
  delete[] height_map_;
}

void Tile::Init(float roughness) {
  srand(static_cast<unsigned int>(time(0)));
  int block_size = size_ - 1;

  // Ecken mit Zufallswerten initialisieren
  height_map_[M(0, 0)] = randf();
  height_map_[M(0, block_size)] = randf();
  height_map_[M(block_size, 0)] = randf();
  height_map_[M(block_size, block_size)] = randf();

  while (block_size > 1) {
    Refine(block_size, roughness);
    block_size = block_size / 2;
  }
}

void Tile::Refine(int block_size, float roughness) {
  int block_size_h = block_size/2;
  float offset_factor = roughness * block_size / size_;

  for (int y = block_size_h; y < size_; y += block_size) {
    for (int x = block_size_h; x < size_; x += block_size) {
      // Lookup der umliegenden Werte (-); o ist Position (x, y)
      // -   -
      //   o
      // -   -
      float nw = height_map_[M(x - block_size_h, y - block_size_h)];
      float ne = height_map_[M(x + block_size_h, y - block_size_h)];
      float sw = height_map_[M(x - block_size_h, y + block_size_h)];
      float se = height_map_[M(x + block_size_h, y + block_size_h)];
      
      // Berechnung der neuen Werte (+)
      // - + -
      // + +
      // -   -
      float center = (nw + ne + sw + se) / 4 + offset_factor * randf();
      height_map_[M(x, y)] = center;
      
      float n = nw + ne + center;
      if (y > block_size_h) {
        n += height_map_[M(x, y - block_size)];
        n /= 4;
      } else {
        n /= 3;
      }
      height_map_[M(x, y - block_size_h)] = n + offset_factor * randf();
      float w = nw + sw + center;
      if (x > block_size_h) {
        w += height_map_[M(x - block_size, y)];
        w /= 4;
      } else {
        w /= 3;
      }
      height_map_[M(x - block_size_h, y)] = w + offset_factor * randf();

      // Edge cases: Berechnung neuer Werte (+) am rechten bzw. unteren Rand
      // -   -
      //     +
      // - + - 
      if (x == size_ - 1 - block_size_h) {
        height_map_[M(x + block_size_h, y)] =
            (ne + se + center) / 3 + offset_factor * randf();
      }
      if (y == size_ - 1 - block_size_h) {
        height_map_[M(x, y + block_size_h)] =
            (sw + se + center) / 3 + offset_factor * randf();
      }
    }
  } 
}

float Tile::GetMinHeight() const {
  static float min = std::numeric_limits<float>::max();
  if (min == std::numeric_limits<float>::max()) {
    for (int i = 0; i < size_*size_; ++i) {
      min = std::min(min, height_map_[i]);
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
    for (int i = 1; i < size_*size_; ++i) {
      max = std::max(max, height_map_[i]);
    }
    if (num_lod_ > 0) {
      for (int dir = 0; dir < 4; ++dir) {
        max = std::max(max, children_[dir]->GetMaxHeight());
      }
    }
  }
  return max;
}

void Tile::SaveImage0(const std::wstring &basename,
                      const std::wstring &extension,
                      float min,
                      float max) const {
  unsigned char *image_data = new unsigned char[3*size_*size_];
  const int num_spots = 8;
  const float spots[num_spots] = {
      -1.0f,
      -0.25f,
       0.0f,
       0.0625f,
       0.125f,
       0.375f,
       0.75f,
       1.0f
  };
  const unsigned char colors[num_spots][3] = {
    { 160, 0, 0 },      // Tiefes Wasser
    { 255, 64, 0 },     // Seichtes Wasser
    { 255, 128, 0 },    // Küste
    { 64, 240, 240 },   // Strand
    { 0, 160, 32 },     // Gras
    { 0, 128, 0 },      // Wald
    { 128, 128, 128 },  // Gestein
    { 255, 255, 255 }   // Schnee
  };
  for (int i = 0; i < size_*size_; ++i) {
    // Wert zwischen -1 und 1
    float value = height_map_[i];
    if (value < spots[0]) {
      image_data[3*i]   = colors[0][0];
      image_data[3*i+1] = colors[0][1];
      image_data[3*i+2] = colors[0][2];
    } else if (value > spots[num_spots - 1]) {
      image_data[3*i]   = colors[num_spots - 1][0];
      image_data[3*i+1] = colors[num_spots - 1][1];
      image_data[3*i+2] = colors[num_spots - 1][2];
    } else {
      for (int j = 0; j < num_spots - 1; ++j) {
        if (spots[j] <= value && value <= spots[j+1]) {
          float interpolated_value = (value - spots[j]) / (spots[j+1] - spots[j]);
          for (int c = 0; c < 3; ++c) {
            image_data[3*i+c] =
                static_cast<unsigned char>(
                    (1 - interpolated_value) * colors[j][c] +
                    interpolated_value * colors[j+1][c]);
          }
          break;
        }
      }
    }
  }
  std::wstring filename(basename);
  filename.append(extension);
  IOTools::SaveImage(filename.c_str(), size_, size_, image_data);

  if (num_lod_ > 0) {
    std::wstring filename;
    filename = basename;
    filename.append(L"0");
    children_[NW]->SaveImage0(filename, extension, min, max);
    filename = basename;
    filename.append(L"1");
    children_[NE]->SaveImage0(filename, extension, min, max);
    filename = basename;
    filename.append(L"2");
    children_[SW]->SaveImage0(filename, extension, min, max);
    filename = basename;
    filename.append(L"3");
    children_[SE]->SaveImage0(filename, extension, min, max);
  }
}

void Tile::SaveImages(const std::wstring &filename) const {
  float min = GetMinHeight();
  float max = GetMaxHeight();
  std::wstring basename(filename);
  basename.erase(basename.rfind('.'), basename.size());
  std::wstring extension(filename, filename.rfind('.'));
  SaveImage0(basename, extension, min, max);
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
  std::ofstream ofs(filename.c_str());
  ofs << "# Terrain file" << std::endl;
  // Vertices & Texturkoordinaten
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x) {
      // Vertex
      ofs << "v ";
      ofs << ((float)x/size_*6-3) << " ";
      ofs << std::max(0.0f, height_map_[M(x,y)]) << " ";
      ofs << ((float)y/size_*6-3) << std::endl;
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
};
