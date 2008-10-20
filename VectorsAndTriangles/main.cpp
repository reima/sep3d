#include "stdafx.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include "Triangle.h"
#include "Vec3.h"

const double EPSILON = 1.0e-10;

void testVec3(void) {
  Vec3 v1(1, 2, 3);
  Vec3 v2(4, 5, 6);

  v1.add(v2);
  assert(v1.isEqual(Vec3(5, 7, 9)));

  v2.subtract(v1);
  assert(v2.isEqual(Vec3(-1, -2, -3)));

  v2.scale(2.0);
  assert(v2.isEqual(Vec3(-2, -4, -6)));
  v1.scale(-0.5);
  assert(v1.isEqual(Vec3(-2.5, -3.5, -4.5)));

  assert(abs(v1.getNorm() - sqrt(38.75)) < EPSILON);
  assert(abs(v2.getNorm() - sqrt(56.0)) < EPSILON);

  v1.normalize();
  assert(abs(v1.getNorm() - 1.0) < EPSILON);
  v2.normalize();
  assert(abs(v2.getNorm() - 1.0) < EPSILON);

  assert(v1.isEqual(v1));
  assert(v2.isEqual(v2));

  v1 = Vec3(1, 2, 3);
  v2 = Vec3(-7, 8, 9);
  
  Vec3 v3 = v1.crossProd(v2);
  assert(v3.isEqual(Vec3(-6, -30, 22)));
  
  Vec3 v4 = v2.crossProd(v1);
  v4.add(v3);
  assert(v4.isEqual(Vec3(0, 0, 0)));

  std::streambuf *psbuf;
  psbuf = std::cout.rdbuf();
  std::stringbuf sb;
  std::cout.rdbuf(&sb);
  v1 = Vec3(0.1, 2.3, 4.5);
  v1.print();
  assert(sb.str() == "(0.1, 2.3, 4.5)");
  std::cout.rdbuf(psbuf);
}

int _tmain(int argc, _TCHAR* argv[])
{
  Vec3 p1(5.4, 3.6, 4.2);
  Vec3 p2(5.9, 4.6, 5.2);
  Vec3 p3(0.2, -0.8, 0.6);

  Vec3 temp1 = p2;
  temp1.add(p3);
  Vec3 temp2 = p2;
  temp2.subtract(p3);

  Triangle tri(p1, temp1, temp2);

  std::cout << "Dreieck: ";
  tri.print();
  std::cout << std::endl;

  std::cout << "Mittelpunkt: ";
  tri.getCenter().print();
  std::cout << std::endl;

  std::cout << "Normale: ";
  tri.getNormal().print();
  std::cout << std::endl;

  return 0;
}

