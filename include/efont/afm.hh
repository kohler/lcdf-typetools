#ifndef AFM_HH
#define AFM_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "metrics.hh"
class LineScanner;
class ErrorHandler;
class AfmMetricsXt;


class AfmReader {
  
  Metrics *_afm;
  AfmMetricsXt *_afm_xt;
  LineScanner &_l;
  ErrorHandler *_errh;
  
  mutable bool _composite_warned;
  mutable bool _metrics_sets_warned;
  mutable int _y_width_warned;
  
  void composite_warning() const;
  void metrics_sets_warning() const;
  void y_width_warning() const;
  void no_match_warning() const;
  
  double &fd(int i)				{ return _afm->fd(i); }
  GlyphIndex find_err(PermString, const char *) const;
  
  void read_char_metric_data() const;
  void read_char_metrics() const;
  void read_kerns() const;
  void read_composites() const;
  
  void read();
  
 public:
  
  AfmReader(LineScanner &, ErrorHandler *);
  ~AfmReader();
  
  bool ok() const				{ return _afm; }
  Metrics *take();
  
};


struct AfmMetricsXt: public MetricsXt {
  
  Vector<PermString> opening_comments;
  PermString notice;
  PermString encoding_scheme;
  
  PermString kind() const			{ return "AFM"; }
  
};

#endif
