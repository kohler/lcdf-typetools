// -*- c-basic-offset: 2 -*-
/*
 * straccum.{cc,hh} -- build up strings with operator<<
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2001-2007 Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "straccum.hh"
#include <cstdarg>
#include <cstdio>

void
StringAccum::make_out_of_memory()
{
  assert(_cap >= 0);
  delete[] _s;
#ifdef HAVE_STRING
  _s = reinterpret_cast<unsigned char *>(const_cast<char *>(String::out_of_memory_string().data()));
#else
  _s = 0;
#endif
  _cap = -1;
  _len = 0;
}

bool
StringAccum::grow(int want)
{
  // can't append to out-of-memory strings
  if (_cap < 0)
    return false;
  
  int ncap = (_cap ? _cap * 2 : 128);
  while (ncap <= want)
    ncap *= 2;
  
  unsigned char *n = new unsigned char[ncap];
  if (!n) {
    make_out_of_memory();
    return false;
  }
  
  if (_s)
    memcpy(n, _s, _cap);
  delete[] _s;
  _s = n;
  _cap = ncap;
  return true;
}

const char *
StringAccum::c_str()
{
  if (_len < _cap || grow(_len))
    _s[_len] = '\0';
  return reinterpret_cast<char *>(_s);
}

#ifdef HAVE_STRING
String
StringAccum::take_string()
{
  int len = length();
  if (len) {
    int capacity = _cap;
    return String::claim_string(take(), len, capacity);
  } else if (out_of_memory())
    return String::out_of_memory_string();
  else
    return String();
}
#endif

StringAccum &
operator<<(StringAccum &sa, long i)
{
  if (char *x = sa.reserve(24)) {
    int len = sprintf(x, "%ld", i);
    sa.forward(len);
  }
  return sa;
}

StringAccum &
operator<<(StringAccum &sa, unsigned long u)
{
  if (char *x = sa.reserve(24)) {
    int len = sprintf(x, "%lu", u);
    sa.forward(len);
  }
  return sa;
}

StringAccum &
operator<<(StringAccum &sa, double d)
{
  if (char *x = sa.reserve(256)) {
    int len = sprintf(x, "%g", d);
    sa.forward(len);
  }
  return sa;
}

StringAccum &
StringAccum::snprintf(int n, const char *format, ...)
{
  va_list val;
  va_start(val, format);
  if (char *x = reserve(n + 1)) {
#ifdef HAVE_VSNPRINTF
    int len = vsnprintf(x, n + 1, format, val);
#else
    int len = vsprintf(x, format, val);
    assert(len <= n);
#endif
    forward(len);
  }
  va_end(val);
  return *this;
}
