#ifndef STRACCUM_HH
#define STRACCUM_HH
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#ifdef HAVE_PERMSTRING
# include "permstr.hh"
#endif
#ifdef HAVE_STRING
# include "string.hh"
#endif

class StringAccum {
  
  unsigned char *_s;
  int _len;
  int _cap;
  
  void grow(int);
  void erase()				{ _s = 0; _len = 0; _cap = 0; }
  
 public:
  
  StringAccum()				: _s(0), _len(0), _cap(0) { }
  explicit StringAccum(int);
  ~StringAccum()			{ if (_s) free(_s); }
  
  char *value() const			{ return (char *)_s; }
  int length() const			{ return _len; }
  
  void clear()				{ _len = 0; }
  
  char *reserve(int);
  void forward(int f)			{ _len += f; assert(_len <= _cap); }
  char *extend(int);
  
  void push(unsigned char);
  void push(int);
  void append(unsigned char);
  void append(int);
  
  void pop(int n = 1)			{ if (_len >= n) _len -= n; }
  
  void take(unsigned char *&s, int &l)	{ s = _s; l = _len; erase(); }
  unsigned char *take_bytes();
  char *take();
  
  // STRING OPERATIONS
  
  char operator[](int i) const	{ assert(i>=0 && i<_len); return (char)_s[i]; }
  char &operator[](int i)	{ assert(i>=0 && i<_len); return (char &)_s[i]; }
  
};


inline
StringAccum::StringAccum(int cap)
  : _s(new unsigned char[cap]), _len(0), _cap(cap)
{
}

inline void
StringAccum::push(unsigned char c)
{
  if (_len >= _cap) grow(_len);
  _s[_len++] = c;
}

inline void
StringAccum::push(int c)
{
  assert(c >= 0 && c < 256);
  push((unsigned char)c);
}

inline void
StringAccum::append(unsigned char c)
{
  push(c);
}

inline void
StringAccum::append(int c)
{
  push(c);
}

inline char *
StringAccum::reserve(int hm)
{
  if (_len + hm > _cap) grow(_len + hm);
  return (char *)(_s + _len);
}

inline char *
StringAccum::extend(int amt)
{
  char *c = reserve(amt);
  _len += amt;
  return c;
}

inline StringAccum &
operator<<(StringAccum &sa, char c)
{
  sa.push(c);
  return sa;
}

StringAccum &operator<<(StringAccum &, int);
StringAccum &operator<<(StringAccum &, unsigned);
StringAccum &operator<<(StringAccum &, double);

inline StringAccum &
operator<<(StringAccum &sa, const char *s)
{
  int len = strlen(s);
  memcpy(sa.extend(len), s, len);
  return sa;
}

#ifdef HAVE_PERMSTRING
inline StringAccum &
operator<<(StringAccum &sa, PermString s)
{
  memcpy(sa.extend(s.length()), s.cc(), s.length());
  return sa;
}
#endif

#ifdef HAVE_STRING
inline StringAccum &
operator<<(StringAccum &sa, const String &s)
{
  memcpy(sa.extend(s.length()), s.data(), s.length());
  return sa;
}
#endif

inline unsigned char *
StringAccum::take_bytes()
{
  unsigned char *str = _s;
  erase();
  return str;
}

inline char *
StringAccum::take()
{
  return (char *)take_bytes();
}

// STRING OPERATIONS

inline bool
operator==(const StringAccum &sa1, const StringAccum &sa2)
{
  return sa1.length() == sa2.length()
    && memcmp(sa1.value(), sa2.value(), sa1.length()) == 0;
}

inline bool
operator==(const StringAccum &sa, const char *cc)
{
  return strcmp(sa.value(), cc) == 0;
}

inline bool
operator==(const char *cc, const StringAccum &sa)
{
  return sa == cc;
}

inline bool
operator!=(const StringAccum &sa1, const StringAccum &sa2)
{
  return !(sa1 == sa2);
}

inline bool
operator!=(const StringAccum &sa, const char *cc)
{
  return !(sa == cc);
}

inline bool
operator!=(const char *cc, const StringAccum &sa)
{
  return !(sa == cc);
}

#endif
