#include <cmath>
#include "Geom2D.h"

#pragma region // Line2D implementation
Line2D::Line2D(const D3DXVECTOR2 &start, const D3DXVECTOR2 &end)
    : start_(start),
      end_(end),
      direction_(end - start) {
  D3DXVec2Normalize(&direction_, &direction_);
  normal_.x = -direction_.y;
  normal_.y =  direction_.x;
  distance_ = D3DXVec2Dot(&normal_, &start_);
}

Line2D::~Line2D(void) {
}

Line2D Line2D::OrthogonalLine(void) const {
  return Line2D(start_, start_ + normal_);
}

Line2D Line2D::ParallelThrough(const D3DXVECTOR2 &point) const {
  return Line2D(point, point + direction_);
}

float Line2D::Distance(const D3DXVECTOR2 &point) const {
  return D3DXVec2Dot(&normal_, &point) - distance_;
}

float Line2D::Angle(const Line2D &line) const {
  return acos(D3DXVec2Dot(&direction_, &line.direction_));
}

D3DXVECTOR2 Line2D::Intersection(const Line2D &line) const {
  float denom = direction_.x * line.direction_.y -
    direction_.y * line.direction_.x;
  float lambda = (-start_.x * line.direction_.y +
    line.start_.x * line.direction_.y +
    start_.y * line.direction_.x -
    line.start_.y * line.direction_.x) / denom;
  return PointAt(lambda);
}

D3DXVECTOR2 Line2D::PointAt(float lambda) const {
  return start_ + lambda * direction_;
}
#pragma endregion

#pragma region // ConvexPolygon2D implementation
ConvexPolygon2D::ConvexPolygon2D(void) {
}

ConvexPolygon2D::~ConvexPolygon2D(void) {
}

void ConvexPolygon2D::AddPoint(const D3DXVECTOR2 &point) {
  points_.push_back(point);
}

void ConvexPolygon2D::AddPoint(const D3DXVECTOR3 &point) {
  points_.push_back(D3DXVECTOR2(point));
}

void ConvexPolygon2D::MakeConvexHull(void) {
  // Gift Wrapping Algorithm
  std::vector<D3DXVECTOR2> convex_hull;
  D3DXVECTOR2 const *start = &points_[0];
  std::vector<D3DXVECTOR2>::const_iterator it1, it2;
  for (it1 = points_.begin() + 1; it1 != points_.end(); ++it1) {
    const D3DXVECTOR2 &point = *it1;
    if (point.y < start->y) start = &point;
  }
  convex_hull.push_back(*start);
  D3DXVECTOR2 const *point_a = start;
  D3DXVECTOR2 const *point_b = NULL;
  D3DXVECTOR2 const *point_c = NULL;
  bool inside = true;
  for (;;) {
    for (it1 = points_.begin(); it1 != points_.end(); ++it1) {
      point_b = &(*it1);
      if (point_a == point_b) continue;
      Line2D ab(*point_a, *point_b);
      inside = true;
      for (it2 = points_.begin(); it2 != points_.end(); ++it2) {
        point_c = &(*it2);
        if (point_a == point_c || point_b == point_c) continue;
        inside = ab.Distance(*point_c) >= -1E-5f;
        if (!inside) break;
      }
      if (inside) break;
    }
    //assert(inside);
    if (!inside) break;
    if (start == point_b) break;
    convex_hull.push_back(*point_b);
    point_a = point_b;
  }
  points_.swap(convex_hull);
}

void ConvexPolygon2D::ClipToLine(const Line2D &line) {
  if (GetPointCount() == 0) return;
  std::vector<D3DXVECTOR2> new_points;
  D3DXVECTOR2 p0 = points_[points_.size()-1];
  bool p0_inside = line.Distance(p0) < 0;
  std::vector<D3DXVECTOR2>::const_iterator it;
  for (it = points_.begin(); it != points_.end(); ++it) {
    const D3DXVECTOR2 &p1 = *it;
    bool p1_inside = line.Distance(p1) < 0;
    if (p0_inside && p1_inside) {
      new_points.push_back(p1);
    } else if (p0_inside && !p1_inside) {
      new_points.push_back(line.Intersection(Line2D(p0, p1)));
    } else if (!p0_inside && p1_inside) {
      new_points.push_back(line.Intersection(Line2D(p0, p1)));
      new_points.push_back(p1);
    } else {
      // do nothing
    }
    p0 = p1;
    p0_inside = p1_inside;
  }
  points_.swap(new_points);
}

void ConvexPolygon2D::ClipToRect(const D3DXVECTOR2 &min, const D3DXVECTOR2 &max) {
  D3DXVECTOR2 minmax(min.x, max.y);
  D3DXVECTOR2 maxmin(max.x, min.y);
  ClipToLine(Line2D(min, minmax));
  ClipToLine(Line2D(minmax, max));
  ClipToLine(Line2D(max, maxmin));
  ClipToLine(Line2D(maxmin, min));
}

void ConvexPolygon2D::FindExtremePoints(const Metric2D &metric,
                                        float *pos_extreme_value,
                                        D3DXVECTOR2 *pos_extreme_point,
                                        float *neg_extreme_value,
                                        D3DXVECTOR2 *neg_extreme_point) const {
  if (points_.size() == 0) return;
  *pos_extreme_value = *neg_extreme_value = 0;
  D3DXVECTOR2 const *pos_extreme_point0 = NULL, *neg_extreme_point0 = NULL;
  std::vector<D3DXVECTOR2>::const_iterator it;
  for (it = points_.begin(); it != points_.end(); ++it) {
    const D3DXVECTOR2 &point = *it;
    float value = metric.Calculate(point);
    if (value > *pos_extreme_value) {
      *pos_extreme_value = value;
      pos_extreme_point0 = &point;
    }
    if (value < *neg_extreme_value) {
      *neg_extreme_value = value;
      neg_extreme_point0 = &point;
    }
  }
  if (pos_extreme_point0) *pos_extreme_point = *pos_extreme_point0;
  if (neg_extreme_point0) *neg_extreme_point = *neg_extreme_point0;
}
#pragma endregion

#pragma region // Metric2D implementations
float LineDistanceMetric2D::Calculate(const D3DXVECTOR2 &point) const {
  return line_->Distance(point);
}

float LineAngleMetric2D::Calculate(const D3DXVECTOR2 &point) const {
  return line_.Angle(Line2D(start_, point)) * (line_.Distance(point) < 0 ? -1 : 1);
}
#pragma endregion
