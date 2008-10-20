#pragma once

class Vec3 {
 public:
  Vec3(double x, double y, double z);
  
  void add(const Vec3 &v);
  void subtract(const Vec3 &v);
  void scale(double f);
  double getNorm() const;
  void normalize();
  bool isEqual(const Vec3 &v) const;
  Vec3 crossProd(const Vec3 &v) const;
  void print() const;

 private:
  double x_, y_, z_;
};
