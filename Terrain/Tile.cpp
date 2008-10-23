#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <limits>
#include "IOTools.h"
#include "Tile.h"

// Die Makros min und max aus windef.h vertragen sich nicht mit std::min,
// std::max, std::numeric_limits<*>::min, std::numeric_limits<*>::max.
#undef min
#undef max

#define M(x,y) (y)*size_+(x)

namespace {

/**
 * Generiert zuf‰llige Flieﬂkommazahl zwischen -1 und 1
 */
inline float randf() {
  return rand() / (RAND_MAX * 0.5f) - 1.0f;
}

}

Tile::Tile(int lod, float roughness)
    : lod_(lod),
      size_((1 << lod) + 1),
      index_buffer_(NULL) {
  height_map_ = new float[size_*size_];
  init(roughness);
}

Tile::Tile(const Tile &t) : lod_(t.lod_), size_(t.size_), index_buffer_(NULL) {
  height_map_ = new float[size_*size_];
  memcpy(height_map_, t.height_map_, size_ * size_ * sizeof(float));
  if (t.index_buffer_ != NULL) {
    initIndexBuffer();
    memcpy(
        index_buffer_,
        t.index_buffer_,
        6*(size_-1)*(size_-1)*sizeof(unsigned int));
  }
}

Tile::~Tile(void) {
  delete[] index_buffer_;
  delete[] height_map_;
}

void Tile::init(float roughness) {
  srand(static_cast<unsigned int>(time(0)));
  int size = size_ - 1;

  // Ecken mit Zufallswerten initialisieren
  height_map_[M(0,    0)]    = randf();
  height_map_[M(0,    size)] = randf();
  height_map_[M(size, 0)]    = randf();
  height_map_[M(size, size)] = randf();  

  while (size > 1) {
    int hsize = size/2;
    float offset_factor = roughness * size / size_;

    for (int y = hsize; y < size_; y += size) {
      for (int x = hsize; x < size_; x += size) {
        /**
         * Lookup der umliegenden Werte (-); o ist Position (x, y)
         * -   -
         *   o
         * -   -
         */
        float nw = height_map_[M(x - hsize, y - hsize)];
        float ne = height_map_[M(x + hsize, y - hsize)];
        float sw = height_map_[M(x - hsize, y + hsize)];
        float se = height_map_[M(x + hsize, y + hsize)];

        /**
         * Berechnung der neuen Werte (+)
         * - + -
         * + +
         * -   -
         */       
        float center = (nw + ne + sw + se) / 4 + offset_factor * randf();
        height_map_[M(x, y)] = center;
        
        float n = nw + ne + center;
        if (y > hsize) {
          n += height_map_[M(x, y - size)];
          n /= 4;
        } else {
          n /= 3;
        }
        height_map_[M(x, y - hsize)] = n + offset_factor * randf();

        float w = nw + sw + center;
        if (x > hsize) {
          w += height_map_[M(x - size, y)];
          w /= 4;
        } else {
          w /= 3;
        }
        height_map_[M(x - hsize, y)] = w + offset_factor * randf();

        /**
         * Edge cases: Berechnung neuer Werte (+) am rechten bzw. unteren Rand
         * -   -
         *     +
         * - + -
         */ 
        if (x == size_ - 1 - hsize) {
          height_map_[M(x + hsize, y)] =
              (ne + se + center) / 3 + offset_factor * randf();
        }
        if (y == size_ - 1 - hsize) {
          height_map_[M(x, y + hsize)] =
              (sw + se + center) / 3 + offset_factor * randf();
        }
      }
    }

    size = hsize;
  }
}

float Tile::getMinHeight() const {
  static float min = std::numeric_limits<float>::max();
  if (min == std::numeric_limits<float>::max()) {
    for (int i = 0; i < size_*size_; ++i) {
      min = std::min(min, height_map_[i]);
    }
  }
  return min;
}

float Tile::getMaxHeight() const {
  static float max = std::numeric_limits<float>::min();
  if (max == std::numeric_limits<float>::min()) {
    for (int i = 1; i < size_*size_; ++i) {
      max = std::max(max, height_map_[i]);
    }
  }
  return max;
}

void Tile::saveImage(const TCHAR *filename) const {
  unsigned char *image_data = new unsigned char[3*size_*size_];
  const float min = getMinHeight();
  const float max = getMaxHeight();
  for (int i = 0; i < size_*size_; ++i) {
    unsigned char color =
        static_cast<unsigned char>((height_map_[i] - min) / (max - min) * 255);
    image_data[3*i] = image_data[3*i+1] = image_data[3*i+2] = color;
  }
  IOTools::SaveImage(filename, size_, size_, image_data, true);
}

void Tile::initIndexBuffer(void) {
  if (index_buffer_ == NULL) {
    index_buffer_ = new unsigned int[6*(size_-1)*(size_-1)];
  }    
}

void Tile::triangulateLines(void)  {
  initIndexBuffer();
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
//  for (int i=0; i<(size_-1)*(size_-1)*6; i+=3)
//    printf("%i %i %i\n",index_buffer[i],index_buffer[i+1],index_buffer[i+2]);
}

void Tile::triangulateZOrder(void) {
  initIndexBuffer();
  int i = 0;
  z_rec(0, 0, size_-1, size_-1, i);  
//  for (int i=0; i<(size_-1)*(size_-1)*6; i+=3)
//    printf("%i %i %i\n",index_buffer[i],index_buffer[i+1],index_buffer[i+2]);
}

void Tile::z_rec(int x1, int y1, int x2, int y2, int &i){
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
 
    z_rec(x1, y1, x12, y12, i);
    z_rec(x12, y1, x2, y12, i);
    z_rec(x1, y12, x12, y2, i);
    z_rec(x12, y12, x2, y2, i);
  }
}

void Tile::saveObj(const TCHAR *filename) const {
  std::ofstream ofs(filename);
  ofs << "# Terrain file" << std::endl;
  for (int y = 0; y < size_; ++y) {
    for (int x = 0; x < size_; ++x) {
      ofs << "v ";
      ofs << ((float)x/size_*2-1) << " ";
      ofs << height_map_[M(x,y)] << " ";
      ofs << ((float)y/size_*2-1) << std::endl;
    }
  }
  if (index_buffer_ != NULL) {
    const int num_triangles = 2 * (size_ - 1) * (size_ - 1);
    for (int i = 0; i < num_triangles; ++i) {
      ofs << "f ";
      ofs << (index_buffer_[3*i]+1) << " ";
      ofs << (index_buffer_[3*i+1]+1) << " ";
      ofs << (index_buffer_[3*i+2]+1) << std::endl;
    }
  }
};
