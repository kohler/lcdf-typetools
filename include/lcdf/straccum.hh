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
  
  bool grow(int);
  void erase()				{ _s = 0; _len = 0; _cap = 0; }
  
  StringAccum(const StringAccum &);
  StringAccum &operator=(const StringAccum &);
  
 public:
  
  StringAccum()				: _s(0), _len(0), _cap(0) { }
  explicit StringAccum(int);
  ~StringAccum()			{ delete[] _s; }

  char *cc();
  char *data() const			{ return (char *)_s; }
  int length() const			{ return _len; }
  
  void clear()				{ _len = 0; }
  
  char *reserve(int);
  void forward(int f)			{ _len += f; assert(_len <= _cap); }
  char *extend(int);

  void push(unsigned char);
  void push(char);
  void push(int);
  void push(const char *, int);
  
  void pop(int n = 1)			{ if (_len >= n) _len -= n; }
  
  void take(unsigned char *&s, int &l)	{ s = _s; l = _len; erase(); }
  unsigned char *take_bytes();
  char *take();
#ifdef HAVE_STRING
  String take_string();
#endif
  
  StringAccum &operator<<(char c);
  StringAccum &operator<<(const char *);
#ifdef HAVE_PERMSTRING
  StringAccum &operator<<(PermString);
#endif
#ifdef HAVE_STRING
  StringAccum &operator<<(const String &);
#endif
  StringAccum &operator<<(const StringAccum &);
  StringAccum &operator<<(int);
  StringAccum &operator<<(unsigned);
  StringAccum &operator<<(double);

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
  if (_len < _cap || grow(_len))
    _s[_len++] = c;
}

inline void
StringAccum::push(char c)
{
  push(static_cast<unsigned char>(c));
}

inline void
StringAccum::push(int c)
{
  push(static_cast<unsigned char>(c));
}

inline void
StringAccum::push(const char *s, int len)
{
  if (char *x = extend(len))
    memcpy(x, s, len);
}

inline char *
StringAccum::reserve(int hm)
{
  if (_len + hm <= _cap || grow(_len + hm))
    return (char *)(_s + _len);
  else
    return 0;
}

inline char *
StringAccum::extend(int amt)
{
  char *c = reserve(amt);
  if (c) _len += amt;
  return c;
}

inline StringAccum &
StringAccum::operator<<(char c)
{
  push(c);
  return *this;
}

inline StringAccum &
StringAccum::operator<<(const char *s)
{
  push(s, strlen(s));
  return *this;
}

#ifdef HAVE_PERMSTRING
inline StringAccum &
StringAccum::operator<<(PermString s)
{
  push(s.cc(), s.length());
  return *this;
}
#endif

#ifdef HAVE_STRING
inline StringAccum &
StringAccum::operator<<(const String &s)
{
  push(s.data(), s.length());
  return *this;
}
#endif

inline StringAccum &
StringAccum::operator<<(const StringAccum &s)
{
  push(s.data(), s.length());
  return *this;
}

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
  return reinterpret_cast<char *>(take_bytes());
}

inline bool
operator==(const StringAccum &sa1, const StringAccum &sa2)
{
  return sa1.length() == sa2.length()
    && memcmp(sa1.data(), sa2.data(), sa1.length());
}

inline bool
operator==(StringAccum &sa1, const char *cc2)
{
  return strcmp(sa1.cc(), cc2) == 0;
}

inline bool
operator==(const char *cc1, StringAccum &sa2)
{
  return strcmp(sa2.cc(), cc1) == 0;
}

inline bool
operator!=(const StringAccum &sa1, const StringAccum &sa2)
{
  return !(sa1 == sa2);
}

inline bool
operator!=(StringAccum &sa1, const char *cc2)
{
  return !(sa1 == cc2);
}

inline bool
operator!=(const char *cc1, StringAccum &sa2)
{
  return !(sa2 == cc1);
}

#endif
