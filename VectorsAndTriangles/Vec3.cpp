#include "StdAfx.h"
#include "Vec3.h"

#include <cmath>

Vec3::Vec3(double x, double y, double z) : x_(x), y_(y), z_(z) {
}

void Vec3::add(const Vec3 &v) {
  x_ += v.x_;
  y_ += v.y_;
  z_ += v.z_;
}

void Vec3::subtract(const Vec3 &v) {
  x_ -= v.x_;
  y_ -= v.y_;
  z_ -= v.z_;
}

void Vec3::scale(double f) {
  x_ *= f;
  y_ *= f;
  z_ *= f;
}

double Vec3::getNorm() const {
  return sqrt(x_*x_ + y_*y_ + z_*z_);
}

void Vec3::normalize() {
  scale(1.0/getNorm());
}

bool Vec3::isEqual(const Vec3 &v) const {
  return x_ == v.x_ && y_ == v.y_ && z_ == v.z_;
}

Vec3 Vec3::crossProd(const Vec3 &v) const {
  return Vec3(y_*v.z_ - z_*v.y_, z_*v.x_ - x_*v.z_, x_*v.y_ - y_*v.x_);
}

void Vec3::print() const {
  std::cout << "(" << x_ << ", " << y_ << ", " << z_ << ")";
}
