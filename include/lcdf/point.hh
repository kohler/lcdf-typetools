#ifndef POINT_HH
#define POINT_HH
#include <cmath>
class Transform;

struct Point {
  
  double x;
  double y;
  
  Point()				{ }
  Point(double xx, double yy)		: x(xx), y(yy) { }
  
  operator bool() const			{ return x || y; }
  
  double squared_length() const;
  double length() const;
  double magnitude() const;
  static double distance(const Point &, const Point &);
  static double dot(const Point &, const Point &);
  static Point midpoint(const Point &, const Point &);
  
  double angle() const;
  
  Point rotated(double) const;
  Point normal() const;
  Point transformed(const Transform &) const;

  bool on_line(const Point &, const Point &, double) const;
  bool on_segment(const Point &, const Point &, double) const;
  
  Point &operator+=(const Point &);
  Point &operator-=(const Point &);
  Point &operator*=(double);
  Point &operator/=(double);
  
  // Point operator+(Point, const Point &);
  // Point operator-(Point, const Point &);
  // Point operator*(Point, double);
  // Point operator/(Point, double);
  // Point operator-(const Point &);
  
  // bool operator==(const Point &, const Point &);
  // bool operator!=(const Point &, const Point &);
  
};

inline double
Point::squared_length() const
{
  return x*x + y*y;
}

inline double
Point::length() const
{
  return sqrt(x*x + y*y);
}

inline double
Point::magnitude() const
{
  return length();
}

inline double
Point::angle() const
{
  return atan2(y, x);
}

inline Point
Point::normal() const
{
  double l = length();
  return (l ? Point(x/l, y/l) : *this);
}

inline Point &
Point::operator+=(const Point &p)
{
  x += p.x;
  y += p.y;
  return *this;
}

inline Point &
Point::operator-=(const Point &p)
{
  x -= p.x;
  y -= p.y;
  return *this;
}

inline Point &
Point::operator*=(double d)
{
  x *= d;
  y *= d;
  return *this;
}

inline Point &
Point::operator/=(double d)
{
  x /= d;
  y /= d;
  return *this;
}

inline bool
operator==(const Point &a, const Point &b)
{
  return a.x == b.x && a.y == b.y;
}

inline bool
operator!=(const Point &a, const Point &b)
{
  return a.x != b.x || a.y != b.y;
}

inline Point
operator+(Point a, const Point &b)
{
  a += b;
  return a;
}

inline Point
operator-(Point a, const Point &b)
{
  a -= b;
  return a;
}

inline Point
operator-(const Point &a)
{
  return Point(-a.x, -a.y);
}

inline Point
operator*(Point a, double scale)
{
  a *= scale;
  return a;
}

inline Point
operator*(double scale, Point a)
{
  a *= scale;
  return a;
}

inline Point
operator/(Point a, double scale)
{
  a /= scale;
  return a;
}

inline double
Point::distance(const Point &a, const Point &b)
{
  return (a - b).length();
}

inline double
Point::dot(const Point &a, const Point &b)
{
  return a.x*b.x + a.y*b.y;
}

#endif
