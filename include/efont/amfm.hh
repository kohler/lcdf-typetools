#ifndef AMFM_HH
#define AMFM_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "metrics.hh"
#include "hashmap.hh"
#include "t1mm.hh"
class LineScanner;
class MetricsFinder;
class Type1Charstring;
class ErrorHandler;


struct AmfmMaster {
  
  PermString font_name;
  PermString family;
  PermString full_name;
  PermString version;
  Vector<double> weight_vector;
  
  bool loaded;
  Metrics *afm;
  
  AmfmMaster()				: loaded(0), afm(0) { }
  
};


struct AmfmPrimaryFont {
  
  Vector<int> design_vector;
  Vector<PermString> labels;
  PermString name;
  AmfmPrimaryFont *next;
  
};


class AmfmMetrics {
  
  MetricsFinder *_finder;
  PermString _directory;
  
  Vector<double> _fdv;
  
  PermString _font_name;
  PermString _family;
  PermString _full_name;
  PermString _weight;
  
  PermString _version;
  PermString _notice;
  Vector<PermString> _opening_comments;
  
  PermString _encoding_scheme;
  Vector<double> _weight_vector;
  
  int _nmasters;
  int _naxes;
  AmfmMaster *_masters;
  Type1MMSpace *_mmspace;
  
  AmfmPrimaryFont *_primary_fonts;
  
  Metrics *_sanity_afm;
  
  static HashMap<PermString, PermString> axis_generic_label;
  static void make_axis_generic_label();
  
  friend class AmfmReader;
  
  AmfmMetrics(const AmfmMetrics &)		{ assert(0); }
  AmfmMetrics &operator=(const AmfmMetrics &)	{ assert(0); return *this; }
  
  AmfmPrimaryFont *find_primary_font(const Vector<double> &design) const;
  
  Metrics *master(int, ErrorHandler *);
  
 public:
  
  AmfmMetrics(MetricsFinder *);
  ~AmfmMetrics();
  
  void read(LineScanner &);
  bool sanity(ErrorHandler *) const;
  
  double &fd(int i) const		{ return _fdv[i]; }
  
  PermString font_name() const		{ return _font_name; }
  
  int naxes() const			{ return _naxes; }
  int nmasters() const			{ return _nmasters; }
  Type1MMSpace *mmspace() const		{ return _mmspace; }
  
  int primary_label_value(int, PermString) const;
  
  Metrics *interpolate(const Vector<double> &design,
		       const Vector<double> &weight, ErrorHandler *);
  
};


class AmfmReader {

  typedef Vector<double> NumVector;
  
  AmfmMetrics *_amfm;
  MetricsFinder *_finder;
  LineScanner &_l;
  Type1MMSpace *_mmspace;
  ErrorHandler *_errh;
  
  double &fd(int i) const		{ return _amfm->fd(i); }
  int naxes() const			{ return _amfm->_naxes; }
  int nmasters() const			{ return _amfm->_nmasters; }

  void check_mmspace();
  
  void no_match_warning() const;
  
  bool read_simple_array(Vector<double> &) const;
  void read_positions() const;
  void read_normalize() const;
  void read_axis_types() const;
  void read_axis(int axis) const;
  void read_master(int master) const;
  void read_primary_fonts() const;
  void read_one_primary_font() const;
  Type1Charstring *conversion_program(Vector<PermString> &) const;
  void read_conversion_programs() const;
  void read();
  
  void read_amcp_file();
  
  AmfmReader(const AmfmReader &, LineScanner &);
  
 public:
  
  AmfmReader(LineScanner &, MetricsFinder *, ErrorHandler *);
  ~AmfmReader();
  
  bool ok() const				{ return _amfm; }
  AmfmMetrics *take();
  
};

#endif
