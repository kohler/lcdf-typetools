#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "point.hh"

int
point::on_line(const point &a, const point &b, double tolerance) const
{
  point c = b - a;
  double d = c.x * (y - a.y) - c.y * (x - a.x);
  return (d * d <= tolerance * tolerance * c.squared_length());
}

int
point::on_segment(const point &a, const point &b, double t) const
{
  double tt;
  point c = b - a;
  if (fabs(c.x) > fabs(c.y))
    tt = (x - a.x) / c.x;
  else if (c.y)
    tt = (y - a.y) / c.y;
  else
    tt = 0;
  if (tt < 0 || tt > 1) return 0;
  return on_line(a, b, t);
}

point
point::along(const point &a, const point &b, double d)
{
  double d2 = point::distance(a, b);
  if (d2 == 0) return a;
  return a + (b - a) * (d / d2);
}
