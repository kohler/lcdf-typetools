#ifndef POINT_HH
#define POINT_HH
#include <math.h>


struct point {
  
  double x;
  double y;

  point()				: x(0), y(0) { }
  point(double xx, double yy)		: x(xx), y(yy) { }
  
  double squared_length() const		{ return x * x + y * y; }
  double length() const			{ return sqrt(squared_length()); }
  
  static point midpoint(const point &, const point &);
  static point fraction_point(const point &, const point &, double);
  
  static double squared_length(const point &);
  static double length(const point &);
  static double distance(const point &, const point &);
  static point along(const point &, const point &, double);
  
  int on_line(const point &, const point &, double) const;
  int on_segment(const point &, const point &, double) const;
  
  friend point operator+(const point &, const point &);
  friend point operator-(const point &, const point &);
  point &operator+=(const point &);
  point &operator-=(const point &);
  friend point operator*(double, const point &);
  friend point operator*(const point &, double);
  friend int operator==(const point &, const point &);
  friend int operator!=(const point &, const point &);
  point &operator*=(double);
  
};


inline point
point::midpoint(const point &a, const point &b)
{
  return point((a.x + b.x) / 2, (a.y + b.y) / 2);
}

inline point
operator+(const point &a, const point &b)
{
  return point(a.x + b.x, a.y + b.y);
}

inline point
operator-(const point &a, const point &b)
{
  return point(a.x - b.x, a.y - b.y);
}

inline point &
point::operator+=(const point &b)
{
  return (*this = *this + b);
}

inline point &
point::operator-=(const point &b)
{
  return (*this = *this - b);
}

inline double
point::squared_length(const point &a)
{
  return a.squared_length();
}

inline double
point::length(const point &a)
{
  return a.length();
}

inline double
point::distance(const point &a, const point &b)
{
  return length(a - b);
}

inline point
operator*(const point &a, double t)
{
  return point(a.x * t, a.y * t);
}

inline point
operator*(double t, const point &a)
{
  return a * t;
}

inline point &
point::operator*=(double t)
{
  return (*this = *this * t);
}

inline int
operator==(const point &a, const point &b)
{
  return (a.x == b.x && a.y == b.y);
}

inline int
operator!=(const point &a, const point &b)
{
  return (a.x != b.x || a.y != b.y);
}

inline point
point::fraction_point(const point &a, const point &b, double d)
{
  return a * (1 - d) + b * d;
}

#endif
