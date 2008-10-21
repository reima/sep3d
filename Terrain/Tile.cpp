#include "Tile.h"

#include <ctime>
#include <cstdlib>

#include "IOTools.h"

Tile::Tile(int lod, float roughness)
    : lod_(lod),
      size_((1 << lod) + 1) {
  height_map_ = new float[size_*size_];
  init(roughness);
}

Tile::~Tile(void) {
  delete[] height_map_;
}

inline float randf() {
  return rand() / (RAND_MAX * 0.5f) - 1.0f;
}

#define M(x,y) (y)*size_+(x)

void Tile::init(float roughness) {
  srand(static_cast<unsigned int>(time(0)));

  // Ecken mit Zufallswerten initialisieren
  height_map_[M(0,         0)]         = randf();
  height_map_[M(0,         size_ - 1)] = randf();
  height_map_[M(size_ - 1, 0)]         = randf();
  height_map_[M(size_ - 1, size_ - 1)] = randf();

  int size = size_ - 1;

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
        
        float n = nw + ne + center;
        if (y > hsize) {
          n += height_map_[M(x, y - size)];
          n /= 4;
        } else {
          n /= 3;
        }
        n += offset_factor * randf();

        float w = nw + sw + center;
        if (x > hsize) {
          w += height_map_[M(x - size, y)];
          w /= 4;
        } else {
          w /= 3;
        }
        w += offset_factor * randf();

        height_map_[M(x, y)] = center;
        height_map_[M(x, y - hsize)] = n;
        height_map_[M(x - hsize, y)] = w;

        /**
         * Edge cases, Berechnung neuer Werte (+) am rechten bzw. unteren Rand
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
  float min = height_map_[0];
  for (int i = 1; i < size_*size_; ++i) {
    min = min(min, height_map_[i]);
  }
  return min;
}

float Tile::getMaxHeight() const {
  float max = height_map_[0];
  for (int i = 1; i < size_*size_; ++i) {
    max = max(max, height_map_[i]);
  }
  return max;
}

void Tile::saveImage(const TCHAR *filename) const {
  unsigned char *image_data = new unsigned char[3*size_*size_];
  float min = getMinHeight();
  float max = getMaxHeight();
  for (int i = 0; i < size_*size_; ++i) {
    unsigned char color =
        static_cast<unsigned char>((height_map_[i] - min) / (max - min) * 255);
    image_data[3*i] = image_data[3*i+1] = image_data[3*i+2] = color;
  }
  IOTools::SaveImage(filename, size_, size_, image_data, true);
}