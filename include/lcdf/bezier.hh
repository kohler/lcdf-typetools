#ifndef BEZIER_HH
#define BEZIER_HH
#include "point.hh"
#include "vector.hh"

class Bezier {
  
  Point _p[4];
  
 public:
  
  Bezier()				{ }
  Bezier(Point p[4]);
  Bezier(const Point &, const Point &, const Point &, const Point &);
  
  const Point *points() const		{ return _p; }
  const Point &point(int i) const	{ assert(i>=0&&i<4); return _p[i]; }
  
  Point eval(double) const;
  
  static void fit(const Vector<Point> &, double, Vector<Bezier> &);
  
};


inline
Bezier::Bezier(Point p[4])
{
  _p[0] = p[0];
  _p[1] = p[1];
  _p[2] = p[2];
  _p[3] = p[3];
}

inline
Bezier::Bezier(const Point &p0, const Point &p1, const Point &p2, const Point &p3)
{
  _p[0] = p0;
  _p[1] = p1;
  _p[2] = p2;
  _p[3] = p3;
}

#endif
