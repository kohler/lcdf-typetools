#ifdef __GNUG__
#pragma implementation "metrics.hh"
#endif
#include "metrics.hh"


Metrics::Metrics()
  : _name_map(-1),
    _scale(1), _fdv(fdLast, Unkdouble),
    _xt_map(0)
{
  _xt.append((MetricsXt *)0);
}


Metrics::Metrics(PermString font_name, PermString full_name, const Metrics &m)
  : _font_name(font_name),
    _family(m._family), _full_name(full_name), _version(m._version),
    _name_map(m._name_map), _names(m._names), _encoding(m._encoding),
    _scale(1), _fdv(fdLast, Unkdouble),
    _pairp(m._pairp),
    _xt_map(0)
{
  reserve_glyphs(m._wdv.count());
  _kernv.resize(m._kernv.count(), Unkdouble);
  _xt.append((MetricsXt *)0);
}


Metrics::~Metrics()
{
  for (int i = 0; i < _xt.count(); i++)
    delete _xt[i];
}


void
Metrics::set_font_name(PermString n)
{
  assert(!_font_name);
  _font_name = n;
}


void
Metrics::reserve_glyphs(int amt)
{
  if (amt <= _wdv.count()) return;
  _wdv.resize(amt, Unkdouble);
  _lfv.resize(amt, Unkdouble);
  _rtv.resize(amt, Unkdouble);
  _tpv.resize(amt, Unkdouble);
  _btv.resize(amt, Unkdouble);
  _encoding.reserve_glyphs(amt);
  _pairp.reserve_glyphs(amt);
}


GlyphIndex
Metrics::add_glyph(PermString n)
{
  if (glyph_count() >= _wdv.count())
    reserve_glyphs(glyph_count() ? glyph_count() * 2 : 64);
  GlyphIndex gi = _names.append(n);
  _name_map.insert(n, gi);
  return gi;
}


static void
set_dimen(Vector<double> &dest, const Vector<double> &src, double scale,
	  bool increment)
{
  int c = src.count();
  if (increment)
    for (int i = 0; i < c; i++)
      dest.at_u(i) += src.at_u(i) * scale;
  else if (scale < 0.9999 || scale > 1.0001)
    for (int i = 0; i < c; i++)
      dest.at_u(i) = src.at_u(i) * scale;
  else
    dest = src;
}

void
Metrics::interpolate_dimens(const Metrics &m, double scale, bool increment)
{
  set_dimen(_fdv, m._fdv, scale, increment);
  set_dimen(_wdv, m._wdv, scale, increment);
  set_dimen(_lfv, m._lfv, scale, increment);
  set_dimen(_rtv, m._rtv, scale, increment);
  set_dimen(_tpv, m._tpv, scale, increment);
  set_dimen(_btv, m._btv, scale, increment);
  set_dimen(_kernv, m._kernv, scale, increment);
}


void
Metrics::add_xt(MetricsXt *mxt)
{
  int n = _xt.append(mxt);
  _xt_map.insert(mxt->kind(), n);
}
