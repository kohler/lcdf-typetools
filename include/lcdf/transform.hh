#ifndef TRANSFORM_HH
#define TRANSFORM_HH
#include "point.hh"
#include <cassert>

class Transform {

  double _m[6];

 public:

  Transform();
  Transform(double, double, double, double, double, double);

  double value(int i) const		{ assert(i>=0&&i<6); return _m[i]; }
  
  void scale(double, double);
  void scale(const Point &p)			{ scale(p.x, p.y); }
  void scale(double d)				{ scale(d, d); }
  void rotate(double);
  void translate(double, double);
  void translate(const Point &p)		{ translate(p.x, p.y); }
  
  inline Transform scaled(double, double) const;
  Transform scaled(const Point &p) const	{ return scaled(p.x, p.y); }
  Transform scaled(double d) const		{ return scaled(d, d); }
  Transform rotated(double) const;
  Transform translated(double, double) const;
  Transform translated(const Point &p) const;

  Point transform(const Point &p) const;
  Transform transformed(const Transform &) const;

  // Transform operator+(Transform, const Point &);
  // Point operator*(const Point &, const Transform &);
  
};


inline Transform
Transform::scaled(double x, double y) const
{
  Transform t(*this);
  t.scale(x, y);
  return t;
}

inline Transform
Transform::rotated(double r) const
{
  Transform t(*this);
  t.rotate(r);
  return t;
}

inline Transform
Transform::translated(double x, double y) const
{
  Transform t(*this);
  t.translate(x, y);
  return t;
}

inline Transform
Transform::translated(const Point &p) const
{
  return translated(p.x, p.y);
}

inline Transform
operator+(Transform t, const Point &p)
{
  t.translate(p);
  return t;
}

inline Point
operator*(const Point &p, const Transform &t)
{
  return t.transform(p);
}

inline Transform
operator*(const Transform &t1, const Transform &t2)
{
  return t1.transformed(t2);
}

inline Point
Point::transformed(const Transform &t) const
{
  return t.transform(*this);
}

#endif
