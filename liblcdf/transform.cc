#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "transform.hh"

Transform::Transform()
{
  _m[0] = _m[4] = 1;
  _m[1] = _m[2] = _m[3] = _m[5] = 0;
}

Transform::Transform(double m0, double m1, double m2,
		     double m3, double m4, double m5)
{
  _m[0] = m0;
  _m[1] = m1;
  _m[2] = m2;
  _m[3] = m3;
  _m[4] = m4;
  _m[5] = m5;
}

Point
Transform::transform(const Point &p) const
{
  Point q;
  q.x = p.x*_m[0] + p.y*_m[1] + _m[2];
  q.y = p.x*_m[3] + p.y*_m[4] + _m[5];
  return q;
}

Transform
Transform::transformed(const Transform &t) const
{
  return Transform(_m[0] * t._m[0] + _m[1] * t._m[3],
		   _m[0] * t._m[1] + _m[1] * t._m[4],
		   _m[0] * t._m[2] + _m[1] * t._m[5] + _m[2],
		   _m[3] * t._m[0] + _m[4] * t._m[3],
		   _m[3] * t._m[1] + _m[4] * t._m[4],
		   _m[3] * t._m[2] + _m[4] * t._m[5] + _m[5]);
}

void
Transform::scale(double x, double y)
{
  _m[0] *= x;
  _m[1] *= y;
  _m[3] *= x;
  _m[4] *= y;
}

void
Transform::rotate(double r)
{
  double c = cos(r);
  double s = sin(r);
  double a = _m[0], b = _m[1];
  _m[0] = a*c + b*s;
  _m[1] = b*c - a*s;
  a = _m[3], b = _m[4];
  _m[3] = a*c + b*s;
  _m[4] = b*c - a*s;
}

void
Transform::translate(double x, double y)
{
  _m[2] += _m[0]*x + _m[1]*y;
  _m[5] += _m[3]*x + _m[4]*y;
}
