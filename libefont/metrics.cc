// -*- related-file-name: "../include/efont/metrics.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/metrics.hh>
namespace Efont {

Metrics::Metrics()
  : _name_map(-1),
    _scale(1), _fdv(fdLast, Unkdouble),
    _xt_map(0),
    _uses(0)
{
  _xt.push_back((MetricsXt *)0);
}


Metrics::Metrics(PermString font_name, PermString full_name, const Metrics &m)
  : _font_name(font_name),
    _family(m._family), _full_name(full_name), _version(m._version),
    _name_map(m._name_map), _names(m._names), _encoding(m._encoding),
    _scale(1), _fdv(fdLast, Unkdouble),
    _pairp(m._pairp),
    _xt_map(0),
    _uses(0)
{
  reserve_glyphs(m._wdv.size());
  _kernv.resize(m._kernv.size(), Unkdouble);
  _xt.push_back((MetricsXt *)0);
}


Metrics::~Metrics()
{
  assert(_uses == 0);
  for (int i = 1; i < _xt.size(); i++)
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
  if (amt <= _wdv.size()) return;
  _wdv.resize(amt, Unkdouble);
  _lfv.resize(amt, Unkdouble);
  _rtv.resize(amt, Unkdouble);
  _tpv.resize(amt, Unkdouble);
  _btv.resize(amt, Unkdouble);
  _encoding.reserve_glyphs(amt);
  _pairp.reserve_glyphs(amt);
  for (int i = 1; i < _xt.size(); i++)
    _xt[i]->reserve_glyphs(amt);
}


GlyphIndex
Metrics::add_glyph(PermString n)
{
  if (nglyphs() >= _wdv.size())
    reserve_glyphs(nglyphs() ? nglyphs() * 2 : 64);
  GlyphIndex gi = _names.size();
  _names.push_back(n);
  _name_map.insert(n, gi);
  return gi;
}


static void
set_dimen(Vector<double> &dest, const Vector<double> &src, double scale,
	  bool increment)
{
  int c = src.size();
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
  int n = _xt.size();
  _xt.push_back(mxt);
  _xt_map.insert(mxt->kind(), n);
  if (_wdv.size() > 0)
    mxt->reserve_glyphs(_wdv.size());
}

}
