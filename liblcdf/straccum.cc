/*
 * straccum.{cc,hh} -- build up strings with operator<<
 * Eddie Kohler
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "straccum.hh"
#include <stdio.h>

bool
StringAccum::grow(int want)
{
  int ncap = (_cap ? _cap * 2 : 128);
  while (ncap <= want)
    ncap *= 2;

  unsigned char *n = new unsigned char[ncap];
  if (!n)
    return false;
  
  if (_s)
    memcpy(n, _s, _cap);
  delete[] _s;
  _s = n;
  _cap = ncap;
  return true;
}

char *
StringAccum::cc()
{
  if (_len < _cap || grow(_len))
    _s[_len] = 0;
  return reinterpret_cast<char *>(_s);
}

#ifdef HAVE_STRING
String
StringAccum::take_string()
{
  int len = length();
  return String::claim_string(take(), len);
}
#endif

StringAccum &
StringAccum::operator<<(int i)
{
  if (char *x = reserve(256)) {
    int len;
    sprintf(x, "%d%n", i, &len);
    forward(len);
  }
  return *this;
}

StringAccum &
StringAccum::operator<<(unsigned u)
{
  if (char *x = reserve(256)) {
    int len;
    sprintf(x, "%u%n", u, &len);
    forward(len);
  }
  return *this;
}

StringAccum &
StringAccum::operator<<(double d)
{
  if (char *x = reserve(256)) {
    int len;
    sprintf(x, "%g%n", d, &len);
    forward(len);
  }
  return *this;
}
