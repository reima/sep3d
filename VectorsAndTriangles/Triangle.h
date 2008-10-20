#pragma once
#include "Vec3.h"

class Triangle
{
 public:
  Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3);
  Vec3 getCenter() const;
  Vec3 getNormal() const;
  void print() const;

 private:
  Vec3 p1_, p2_, p3_;
};
