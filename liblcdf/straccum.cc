#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __GNUG__
# pragma implementation "straccum.hh"
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
  int len;
  sprintf(sa.reserve(256), "%d%n", i, &len);
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
