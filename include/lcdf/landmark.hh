#ifndef LANDMARK_HH
#define LANDMARK_HH
#include "permstr.hh"
#include "string.hh"

class Landmark {
  
  PermString _file;
  unsigned _line;
  
 public:

  Landmark()				: _file(0), _line(~0U) { }
  explicit Landmark(PermString f)	: _file(f), _line(~0U) { }
  Landmark(PermString f, unsigned l)	: _file(f), _line(l) { }
  
  operator bool() const			{ return _file; }
  bool has_line() const			{ return _line != ~0U; }
  
  PermString file() const		{ return _file; }
  unsigned line() const			{ return _line; }
  
  Landmark next_line() const;
  Landmark whole_file() const		{ return Landmark(_file); }
  
  operator String() const;
  
};

Landmark operator+(const Landmark &, int);

inline Landmark
Landmark::next_line() const
{
  return *this + 1;
}

#endif
