#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "point.hh"

Point
Point::rotated(double rotation) const
{
  double r = length();
  double theta = angle() + rotation;
  return Point(r * cos(theta), r * sin(theta));
}
