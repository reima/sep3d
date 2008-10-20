#include "StdAfx.h"
#include "Triangle.h"

Triangle::Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
    : p1_(p1), p2_(p2), p3_(p3) {
}

Vec3 Triangle::getCenter() const {
  Vec3 v = p1_;
  v.add(p2_);
  v.add(p3_);
  v.scale(1.0/3.0);
  return v;
}

Vec3 Triangle::getNormal() const {
  Vec3 dir1 = p2_;
  dir1.subtract(p1_);
  Vec3 dir2 = p3_;
  dir2.subtract(p2_);
  Vec3 normal = dir1.crossProd(dir2);
  normal.normalize();
  return normal;
}

void Triangle::print() const {
  std::cout << "[ ";
  p1_.print();
  std::cout << ", ";
  p2_.print();
  std::cout << ", ";
  p3_.print();
  std::cout << " ]";
}
