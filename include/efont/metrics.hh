#ifndef METRICS_HH
#define METRICS_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "permstr.hh"
#include "vector.hh"
#include "hashmap.hh"
#include "encoding.hh"
#include "pairop.hh"
class MetricsXt;
typedef int GlyphIndex;

// Allow unknown doubles to have some `fuzz' -- so if an unknown double
// is a bit off from the canonical Unkdouble value, we'll still recognize
// it as unknown. (Useful for interpolation.)
const double Unkdouble		= -9.79797e97;
const double MinKnowndouble	= -9.69696e97;
#define KNOWN(d)		((d) >= MinKnowndouble)

class Metrics {
  
  PermString _font_name;
  
  PermString _family;
  PermString _full_name;
  PermString _weight;
  PermString _version;
  
  HashMap<PermString, GlyphIndex> _name_map;
  Vector<PermString> _names;
  Encoding8 _encoding;
  
  double _scale;
  
  Vector<double> _fdv;
  Vector<double> _wdv;
  Vector<double> _lfv;
  Vector<double> _rtv;
  Vector<double> _tpv;
  Vector<double> _btv;
  
  PairProgram _pairp;
  Vector<double> _kernv;
  
  HashMap<PermString, int> _xt_map;
  Vector<MetricsXt *> _xt;
  
 public:
  
  Metrics();
  Metrics(PermString font_name, PermString full_name, const Metrics &);
  ~Metrics();
  
  // GLOBALS
  
  PermString font_name() const		{ return _font_name; }
  PermString family() const		{ return _family; }
  PermString full_name() const		{ return _full_name; }
  PermString weight() const		{ return _weight; }
  PermString version() const		{ return _version; }
  
  void set_font_name(PermString);
  void set_family(PermString s)		{ _family = s; }
  void set_full_name(PermString s)	{ _full_name = s; }
  void set_weight(PermString s)		{ _weight = s; }
  void set_version(PermString s)	{ _version = s; }
  
  // GLYPHS
  
  int glyph_count() const		{ return _names.count(); }
  PermString name(GlyphIndex gi) const	{ return _names[gi]; }
  GlyphIndex find(PermString n) const	{ return _name_map[n]; }
  
  int code(GlyphIndex gi) const		{ return _encoding.code(gi); }
  GlyphIndex find_code(int c) const	{ return _encoding.find_code(c); }
  
  GlyphIndex add_glyph(PermString);
  void reserve_glyphs(int);
  void set_code(GlyphIndex gi, int c)	{ _encoding.set_code(gi, c); }
  
  // DIMENSIONS
  
  double scale() const			{ return _scale; }
  void set_scale(double d)		{ _scale = d; }
  
  int fd_count() const			{ return _fdv.count(); }
  int kv_count() const			{ return _kernv.count(); }
  
  double &fd(int i) const		{ return _fdv[i]; }
  double &wd(int i) const		{ return _wdv[i]; }
  double &lf(int i) const		{ return _lfv[i]; }
  double &rt(int i) const		{ return _rtv[i]; }
  double &tp(int i) const		{ return _tpv[i]; }
  double &bt(int i) const		{ return _btv[i]; }
  double &kv(int i) const		{ return _kernv[i]; }
  
  int add_kv(double d)			{ return _kernv.append(d); }
  
  void interpolate_dimens(const Metrics &, double, bool increment);
  
  // PAIR PROGRAM
  
  PairProgram *pair_program()			{ return &_pairp; }
  const PairProgram *pair_program() const	{ return &_pairp; }
  bool add_kern(GlyphIndex, GlyphIndex, int);
  bool add_lig(GlyphIndex, GlyphIndex, GlyphIndex, int = opLigSimple);

  // EXTENSIONS

  MetricsXt *find_xt(PermString name) const	{ return _xt[_xt_map[name]]; }
  void add_xt(MetricsXt *);
  
};


class MetricsXt {
  
 public:
  
  MetricsXt()					{ }
  virtual ~MetricsXt()				{ }
  
  virtual PermString kind() const = 0;
  
};


enum FontDimensionDefs {
  
  fdCapHeight = 0,
  fdXHeight,
  fdAscender,
  fdDescender,
  
  fdItalicAngle,
  fdUnderlinePosition,
  fdUnderlineThickness,
  
  fdFontBBllx,
  fdFontBBlly,
  fdFontBBurx,
  fdFontBBury,
  
  fdStdHW,
  fdStdVW,
  
  fdLast
  
};


inline bool
Metrics::add_kern(GlyphIndex g1, GlyphIndex g2, int ki)
{
  return _pairp.add_kern(g1, g2, ki);
}

inline bool
Metrics::add_lig(GlyphIndex g1, GlyphIndex g2, GlyphIndex gr, int kind)
{
  return _pairp.add_lig(g1, g2, gr, kind);
}

#endif
