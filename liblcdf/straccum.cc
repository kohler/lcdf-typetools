#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "straccum.hh"
#include <stdio.h>

StringAccum::StringAccum(const StringAccum &sa)
  : _s((unsigned char *)malloc(sa._cap)), _len(sa._len), _cap(sa._cap)
{
  // try to copy (sa._len+1) bytes in case someone called cc()
  memcpy(_s, sa._s, (sa._cap > sa._len ? sa._len + 1 : sa._len));
}

void
StringAccum::grow(int want)
{
  if (_cap)
    _cap *= 2;
  else
    _cap = 128;
  while (_cap <= want)
    _cap *= 2;
  
  if (_s)
    _s = (unsigned char *)realloc(_s, _cap);
  else
    _s = (unsigned char *)malloc(_cap);
}

char *
StringAccum::cc()
{
  if (_len >= _cap) grow(_len);
  _s[_len] = 0;
  return value();
}

StringAccum &
StringAccum::operator=(const StringAccum &sa)
{
  if (&sa != this) {
    if (sa._len >= _cap) grow(sa._len);
    memcpy(_s, sa._s, sa._len + 1);
    _len = sa._len;
  }
  return *this;
}

StringAccum &
operator<<(StringAccum &sa, int i)
{
  int len;
  sprintf(sa.reserve(256), "%d%n", i, &len);
  sa.forward(len);
  return sa;
}

StringAccum &
operator<<(StringAccum &sa, unsigned u)
{
  int len;
  sprintf(sa.reserve(256), "%u%n", u, &len);
  sa.forward(len);
  return sa;
}

StringAccum &
operator<<(StringAccum &sa, double d)
{
  int len;
  sprintf(sa.reserve(256), "%g%n", d, &len);
  sa.forward(len);
  return sa;
}
