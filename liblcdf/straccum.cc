#ifdef __GNUG__
#pragma implementation "straccum.hh"
#endif
#include "straccum.hh"
#include <stdio.h>


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


StringAccum &
operator<<(StringAccum &sa, int i)
{
#ifdef BAD_SPRINTF
  char *s = sa.reserve(256);
  sprintf(s, "%d", i);
  int len = strlen(s);
#else
  int len = sprintf(sa.reserve(256), "%d", i);
#endif
  sa.forward(len);
  return sa;
}

StringAccum &
operator<<(StringAccum &sa, double d)
{
#ifdef BAD_SPRINTF
  char *s = sa.reserve(256);
  sprintf(s, "%g", d);
  int len = strlen(s);
#else
  int len = sprintf(sa.reserve(256), "%g", d);
#endif
  sa.forward(len);
  return sa;
}
