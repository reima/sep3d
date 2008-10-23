#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <limits>
#include "IOTools.h"
#include "Tile.h"
#include <cstdio>

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
      size_((1 << lod) + 1) {
  height_map_ = new float[size_*size_];
  index_buffer = new unsigned[(size_-1)*(size_-1)*6]; //2 dreiecke zwischen 4 punkten
  buffer_index = 0;
  init(roughness);
}

Tile::Tile(const Tile &t) : lod_(t.lod_), size_(t.size_) {
  height_map_ = new float[size_*size_];
  memcpy(height_map_, t.height_map_, size_ * size_ * sizeof(float));
}

Tile::~Tile(void) {
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
  float min = getMinHeight();
  float max = getMaxHeight();
  for (int i = 0; i < size_*size_; ++i) {
    unsigned char color =
        static_cast<unsigned char>((height_map_[i] - min) / (max - min) * 255);
    image_data[3*i] = image_data[3*i+1] = image_data[3*i+2] = color;
  }
  IOTools::SaveImage(filename, size_, size_, image_data, true);
}

void Tile::triangulate_lines(void) const {
   //dreieck 1 links oben, dreieck2 rechts unten
  
  int size=size_-1;

  for(int y=0; y<size_; y++)
    for(int x=0; x<size_; x++){
      index_buffer[(y*size+x)*6   ]= M(x, y); //1.dreieck links oben
      index_buffer[(y*size+x)*6 +1]= M(x+1, y); //1.dreieck rechts oben
      index_buffer[(y*size+x)*6 +2]= M(x, y+1); //1.dreieck links unten
      index_buffer[(y*size+x)*6 +3]= M(x+1, y); //2.dreieck rechts oben
      index_buffer[(y*size+x)*6 +4]= M(x+1, y+1); //2.dreieck rechts unten
      index_buffer[(y*size+x)*6 +5]= M(x, y+1); //2.dreieck links unten
    }
   // for (int i=0; i<(size_-1)*(size_-1)*6; i+=3)
   //   printf("%i %i %i\n",index_buffer[i],index_buffer[i+1],index_buffer[i+2]);
}

void Tile::triangulate_z(void) {

  
  z_rec(0,0,int((size_-1)/2),int((size_-1)/2));
  z_rec(int((size_-1)/2),0,(size_-1),int((size_-1)/2));
  z_rec(0,int((size_-1)/2),int((size_-1)/2),size_-1);
  z_rec(int((size_-1)/2),int((size_-1)/2),size_-1,size_-1);

//  for (int i=0; i<(size_-1)*(size_-1)*6; i+=3)
//    printf("%i %i %i\n",index_buffer[i],index_buffer[i+1],index_buffer[i+2]);

}

void Tile::z_rec(int x, int y, int x2, int y2){
  // wenn wir weit genug recursive rein sind, die dreiecke erzeugen
  if (x+1==x2) { 
    index_buffer[buffer_index++] = M(x, y); //1.dreieck links oben
    index_buffer[buffer_index++] = M(x+1, y); //1.dreieck rechts oben
    index_buffer[buffer_index++] = M(x, y+1); //1.dreieck links unten
    index_buffer[buffer_index++] = M(x+1, y); //2.dreieck rechts oben
    index_buffer[buffer_index++] = M(x+1, y+1); //2.dreieck rechts unten
    index_buffer[buffer_index++] = M(x, y+1); //2.dreieck links unten
  }
 
  //ansonsten z auf gegebenes viereck anwenden
  else {
    int x12= int((x+x2)/2);
    int y12= int((y+y2)/2);
 
    z_rec(x, y, x12 , y12);
    z_rec(x12, y, x2, y12);
    z_rec(x, y12, x12, y2);
    z_rec(x12, y12, x2, y2);
  }
}