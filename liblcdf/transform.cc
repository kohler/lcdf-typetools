// -*- related-file-name: "../include/lcdf/transform.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/transform.hh>

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


void
Transform::add_scale(double x, double y)
{
    _m[0] *= x;
    _m[1] *= y;
    _m[3] *= x;
    _m[4] *= y;
}

void
Transform::add_rotate(double r)
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
Transform::add_translate(double x, double y)
{
    _m[2] += _m[0]*x + _m[1]*y;
    _m[5] += _m[3]*x + _m[4]*y;
}

Transform
Transform::transform(const Transform &t) const
{
    return Transform(_m[0] * t._m[0] + _m[1] * t._m[3],
		     _m[0] * t._m[1] + _m[1] * t._m[4],
		     _m[0] * t._m[2] + _m[1] * t._m[5] + _m[2],
		     _m[3] * t._m[0] + _m[4] * t._m[3],
		     _m[3] * t._m[1] + _m[4] * t._m[4],
		     _m[3] * t._m[2] + _m[4] * t._m[5] + _m[5]);
}


Point &
operator*=(Point &p, const Transform &t)
{
    double x = p.x;
    p.x = x*t._m[0] + p.y*t._m[1] + t._m[2];
    p.y = x*t._m[3] + p.y*t._m[4] + t._m[5];
    return p;
}

Point
operator*(const Point &p, const Transform &t)
{
    return Point(p.x*t._m[0] + p.y*t._m[1] + t._m[2],
		 p.x*t._m[3] + p.y*t._m[4] + t._m[5]);
}

Bezier &
operator*=(Bezier &b, const Transform &t)
{
    b.mpoint(0) *= t;
    b.mpoint(1) *= t;
    b.mpoint(2) *= t;
    b.mpoint(3) *= t;
    return b;
}

Bezier
operator*(const Bezier &b, const Transform &t)
{
    return Bezier(b.point(0) * t, b.point(1) * t, b.point(2) * t, b.point(3) * t);
}
