#ifndef AFMW_HH
#define AFMW_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "afm.hh"
#include <stdio.h>

class AfmWriter {

  Metrics *_m;
  AfmMetricsXt *_afm_xt;
  FILE *_f;
  
  void write_prologue() const;
  void write_char_metric_data(GlyphIndex, int) const;
  void write_kerns() const;

  double fd(int i) const		{ return _m->fd(i); }
  
 public:
  
  AfmWriter(Metrics *, FILE *);
  
  void write();
  
};

#endif
