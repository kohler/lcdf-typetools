// -*- related-file-name: "../include/lcdf/point.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lcdf/point.hh>

Point
Point::rotated(double rotation) const throw ()
{
    double r = length();
    double theta = angle() + rotation;
    return Point(r * cos(theta), r * sin(theta));
}

Point
Point::midpoint(const Point &a, const Point &b) throw ()
{
    return Point((a.x + b.x)/2, (a.y + b.y)/2);
}

bool
Point::on_line(const Point &a, const Point &b, double tolerance) const throw ()
{
    Point c = b - a;
    double d = c.x * (y - a.y) - c.y * (x - a.x);
    return (d * d <= tolerance * tolerance * c.squared_length());
}

bool
Point::on_segment(const Point &a, const Point &b, double t) const throw ()
{
    double tt;
    Point c = b - a;
    if (fabs(c.x) > fabs(c.y))
	tt = (x - a.x) / c.x;
    else if (c.y)
	tt = (y - a.y) / c.y;
    else
	tt = 0;
    if (tt < 0 || tt > 1)
	return 0;
    return on_line(a, b, t);
}
