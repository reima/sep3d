#pragma once
#include <vector>
#include "DXUT.h"

class Line2D {
 public:
  Line2D(const D3DXVECTOR2 &start, const D3DXVECTOR2 &end);
  ~Line2D(void);

  Line2D OrthogonalLine(void) const;
  Line2D ParallelThrough(const D3DXVECTOR2 &point) const;
  float Distance(const D3DXVECTOR2 &point) const;
  float Angle(const Line2D &line) const;
  D3DXVECTOR2 Intersection(const Line2D &line) const;
  D3DXVECTOR2 PointAt(float lambda) const;

 private:
  D3DXVECTOR2 start_;
  D3DXVECTOR2 end_;
  D3DXVECTOR2 direction_;
  D3DXVECTOR2 normal_;
  float distance_;
};

class Metric2D {
 public:
  virtual ~Metric2D() {};
  virtual float Calculate(const D3DXVECTOR2 &point) const = 0;
};

class LineDistanceMetric2D : public Metric2D {
 public:
  LineDistanceMetric2D(const Line2D *line) : line_(line) {};
  virtual float Calculate(const D3DXVECTOR2 &point) const;
 private:
  const Line2D *line_;
};

class LineAngleMetric2D : public Metric2D {
 public:
  LineAngleMetric2D(const D3DXVECTOR2 &start, const D3DXVECTOR2 &end)
    : start_(start), line_(start, end) {};
  virtual float Calculate(const D3DXVECTOR2 &point) const;
 private:
  const D3DXVECTOR2 start_;
  const Line2D line_;
};

class ConvexPolygon2D {
 public:
  ConvexPolygon2D(void);
  ~ConvexPolygon2D(void);

  void AddPoint(const D3DXVECTOR2 &point);
  void AddPoint(const D3DXVECTOR3 &point);
  void MakeConvexHull(void);
  UINT GetPointCount(void) { return points_.size(); }

  void ClipToLine(const Line2D &line);
  void ClipToRect(const D3DXVECTOR2 &min, const D3DXVECTOR2 &max);

  void FindExtremePoints(const Metric2D &metric,
                         float *pos_extreme_value,
                         D3DXVECTOR2 *pos_extreme_point,
                         float *neg_extreme_value,
                         D3DXVECTOR2 *neg_extreme_point) const;

  const std::vector<D3DXVECTOR2> &GetPoints(void) const { return points_; }

 private:
  std::vector<D3DXVECTOR2> points_;
};
