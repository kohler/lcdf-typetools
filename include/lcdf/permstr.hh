#ifndef PERMSTR_HH
#define PERMSTR_HH
#ifdef __GNUG__
#pragma interface
#endif
#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

class PermString {
  
  struct Doodad {
    Doodad *next;
    int length;
    char data[2];
  };
  
  char *_rep;
  
  PermString(Doodad *d)			: _rep(d->data) { }
  Doodad *doodad() const { return (Doodad *)(_rep - offsetof(Doodad, data)); }
  
  class Initializer;
  friend class PermString::Initializer;
  static void static_initialize();
  
 public:
  
  typedef Doodad *Capsule;
  
  PermString()				: _rep(0) { }
  PermString(int i)			: _rep(0) { if (i) assert(0); }
  explicit PermString(char c);
  PermString(const char *);
  PermString(const char *, int);
  
  operator bool() const			{ return _rep != 0; }
  int length() const			{ return doodad()->length; }
  
  friend bool operator==(PermString, PermString);
  friend bool operator!=(PermString, PermString);
  
  int hashcode() const			{ return (int)_rep; }
  
  const char *cc() const		{ return _rep; }
  operator const char *() const		{ return _rep; }
  
  Capsule capsule() const		{ return doodad(); }
  static PermString decapsule(Capsule c) { return PermString(c); }
  
  friend PermString permprintf(const char *, ...);
  friend PermString vpermprintf(const char *, va_list);
  friend PermString permcat(PermString, PermString);
  friend PermString permcat(PermString, PermString, PermString);
  
  // Declare a PermString::Initializer in any file in which you declare
  // static global PermStrings.
  struct Initializer { Initializer(); };
  
};


inline bool
operator==(PermString a, PermString b)
{
  return a._rep == b._rep;
}

inline bool
operator==(PermString a, const char *b)
{
  return a == PermString(b);
}

inline bool
operator==(const char *a, PermString b)
{
  return PermString(a) == b;
}

inline bool
operator!=(PermString a, PermString b)
{
  return a._rep != b._rep;
}

inline bool
operator!=(PermString a, const char *b)
{
  return a != PermString(b);
}

inline bool
operator!=(const char *a, PermString b)
{
  return PermString(a) != b;
}

#endif
