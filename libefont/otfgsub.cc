// -*- related-file-name: "../include/efont/otfgsub.hh" -*-

/* otfgsub.{cc,hh} -- OpenType GSUB table
 *
 * Copyright (c) 2003 Eddie Kohler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otfgsub.hh>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace Efont { namespace OpenType {

void
Substitution::clear(Substitute &s, uint8_t &t)
{
    switch (t) {
      case T_GLYPHS:
	delete[] s.gids;
	break;
      case T_COVERAGE:
	delete s.coverage;
	break;
    }
    t = T_NONE;
}

void
Substitution::assign(Substitute &s, uint8_t &t, Glyph gid)
{
    clear(s, t);
    s.gid = gid;
    t = T_GLYPH;
}

void
Substitution::assign(Substitute &s, uint8_t &t, int ngids, const Glyph *gids)
{
    clear(s, t);
    assert(ngids > 0);
    if (ngids == 1) {
	s.gid = gids[0];
	t = T_GLYPH;
    } else {
	s.gids = new Glyph[ngids + 1];
	s.gids[0] = ngids;
	memcpy(s.gids + 1, gids, ngids * sizeof(Glyph));
	t = T_GLYPHS;
    }
}

void
Substitution::assign(Substitute &s, uint8_t &t, const Coverage &coverage)
{
    clear(s, t);
    s.coverage = new Coverage(coverage);
    t = T_COVERAGE;
}

void
Substitution::assign(Substitute &s, uint8_t &t, const Substitute &os, uint8_t ot)
{
    assert(&s != &os);
    switch (ot) {
      case T_NONE:
	clear(s, t);
	break;
      case T_GLYPH:
	assign(s, t, os.gid);
	break;
      case T_GLYPHS:
	assign(s, t, os.gids[0], os.gids + 1);
	break;
      case T_COVERAGE:
	assign(s, t, *os.coverage);
	break;
      default:
	assert(0);
    }
}

Substitution::Substitution(const Substitution &o)
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_NONE), _right_is(T_NONE)
{
    assign(_left, _left_is, o._left, o._left_is);
    assign(_in, _in_is, o._in, o._in_is);
    assign(_out, _out_is, o._out, o._out_is);
    assign(_right, _right_is, o._right, o._right_is);
}

Substitution::Substitution(Glyph in, Glyph out)
    : _left_is(T_NONE), _in_is(T_GLYPH), _out_is(T_GLYPH), _right_is(T_NONE)
{
    _in.gid = in;
    _out.gid = out;
}

Substitution::Substitution(Glyph in, const Vector<Glyph> &out, bool is_alternate)
    : _left_is(T_NONE), _in_is(T_GLYPH), _out_is(T_NONE), _right_is(T_NONE),
      _alternate(is_alternate)
{
    assert(out.size() > 0);
    _in.gid = in;
    assign(_out, _out_is, out.size(), &out[0]);
}

Substitution::Substitution(Glyph in1, Glyph in2, Glyph out)
    : _left_is(T_NONE), _in_is(T_GLYPHS), _out_is(T_GLYPH), _right_is(T_NONE)
{
    _in.gids = new Glyph[3];
    _in.gids[0] = 2;
    _in.gids[1] = in1;
    _in.gids[2] = in2;
    _out.gid = out;
}

Substitution::Substitution(const Vector<Glyph> &in, Glyph out)
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_GLYPH), _right_is(T_NONE)
{
    assert(in.size() > 0);
    assign(_in, _in_is, in.size(), &in[0]);
    _out.gid = out;
}

Substitution::Substitution(int nin, const Glyph *in, Glyph out)
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_GLYPH), _right_is(T_NONE)
{
    assert(nin > 0);
    assign(_in, _in_is, nin, in);
    _out.gid = out;
}

Substitution::Substitution(Context c, Glyph g)
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_NONE), _right_is(T_NONE)
{
    assert(c == C_LEFT || c == C_RIGHT);
    if (c == C_LEFT)
	_left_is = T_GLYPH, _left.gid = g;
    else
	_right_is = T_GLYPH, _right.gid = g;
}

Substitution::~Substitution()
{
    clear(_left, _left_is);
    clear(_in, _in_is);
    clear(_out, _out_is);
    clear(_right, _right_is);
}

Substitution &
Substitution::operator=(const Substitution &o)
{
    if (&o != this) {
	assign(_left, _left_is, o._left, o._left_is);
	assign(_in, _in_is, o._in, o._in_is);
	assign(_out, _out_is, o._out, o._out_is);
	assign(_right, _right_is, o._right, o._right_is);
    }
    return *this;
}

bool
Substitution::substitute_in(const Substitute &s, uint8_t t, const Coverage &c)
{
    switch (t) {
      case T_NONE:
	return true;
      case T_GLYPH:
	return c.covers(s.gid);
      case T_GLYPHS:
	for (int i = 1; i <= s.gids[0]; i++)
	    if (!c.covers(s.gids[i]))
		return false;
	return true;
      case T_COVERAGE:
	return *s.coverage <= c;
      default:
	assert(0);
	return false;
    }
}

bool
Substitution::substitute_in(const Substitute &s, uint8_t t, const GlyphSet &gs)
{
    switch (t) {
      case T_NONE:
	return true;
      case T_GLYPH:
	return gs.covers(s.gid);
      case T_GLYPHS:
	for (int i = 1; i <= s.gids[0]; i++)
	    if (!gs.covers(s.gids[i]))
		return false;
	return true;
      case T_COVERAGE:
	for (Coverage::iterator i = s.coverage->begin(); i; i++)
	    if (!gs.covers(*i))
		return false;
	return true;
      default:
	assert(0);
	return false;
    }
}

bool
Substitution::context_in(const Coverage &c) const
{
    return substitute_in(_left, _left_is, c)
	&& substitute_in(_in, _in_is, c)
	&& substitute_in(_right, _right_is, c);
}

bool
Substitution::context_in(const GlyphSet &gs) const
{
    return substitute_in(_left, _left_is, gs)
	&& substitute_in(_in, _in_is, gs)
	&& substitute_in(_right, _right_is, gs);
}

Glyph
Substitution::extract_glyph(const Substitute &s, uint8_t t) throw ()
{
    return (t == T_GLYPH ? s.gid : 0);
}

Glyph
Substitution::extract_glyph_0(const Substitute &s, uint8_t t) throw ()
{
    switch (t) {
      case T_GLYPH:
	return s.gid;
      case T_GLYPHS:
	return (s.gids[0] >= 1 ? s.gids[1] : 0);
      case T_COVERAGE:
	for (Coverage::iterator ci = s.coverage->begin(); ci; ci++)
	    return *ci;
	return 0;
      default:
	return 0;
    }
}

bool
Substitution::extract_glyphs(const Substitute &s, uint8_t t, Vector<Glyph> &v) throw ()
{
    switch (t) {
      case T_GLYPH:
	v.push_back(s.gid);
	return true;
      case T_GLYPHS:
	for (int i = 1; i <= s.gids[0]; i++)
	    v.push_back(s.gids[i]);
	return true;
      case T_COVERAGE:
	for (Coverage::iterator i = s.coverage->begin(); i; i++)
	    v.push_back(*i);
	return true;
      default:
	return false;
    }
}

const Glyph *
Substitution::extract_glyphptr(const Substitute &s, uint8_t t) throw ()
{
    switch (t) {
      case T_GLYPH:
	return &s.gid;
      case T_GLYPHS:
	return &s.gids[1];
      default:
	return 0;
    }
}

int
Substitution::extract_nglyphs(const Substitute &s, uint8_t t, bool coverage_ok) throw ()
{
    switch (t) {
      case T_GLYPH:
	return 1;
      case T_GLYPHS:
	return s.gids[0];
      case T_COVERAGE:
	return (coverage_ok ? 1 : 0);
      default:
	return 0;
    }
}

bool
Substitution::matches(const Substitute &s, uint8_t t, int pos, Glyph g) throw ()
{
    switch (t) {
      case T_GLYPH:
	return (pos == 0 && s.gid == g);
      case T_GLYPHS:
	return (pos >= 0 && pos < s.gids[0] && s.gids[1 + pos] == g);
      case T_COVERAGE:
	return (pos == 0 && s.coverage->covers(g));
      default:
	return false;
    }
}

bool
Substitution::is_noop() const
{
    return (_in_is == T_GLYPH && _out_is == T_GLYPH && _in.gid == _out.gid)
	|| (_in_is == T_GLYPHS && _out_is == T_GLYPHS
	    && _in.gids[0] == _out.gids[0]
	    && memcmp(_in.gids, _out.gids, (_in.gids[0] + 1) * sizeof(Glyph)) == 0);
}

void
Substitution::assign_append(Substitute &s, uint8_t &t, const Substitute &ls, uint8_t lt, const Substitute &rs, uint8_t rt)
{
    clear(s, t);
    if (lt == T_NONE)
	assign(s, t, rs, rt);
    else if (rt == T_NONE)
	assign(s, t, ls, lt);
    else if (lt != T_COVERAGE && rt != T_COVERAGE) {
	int nl = extract_nglyphs(ls, lt, false);
	int nr = extract_nglyphs(rs, rt, false);
	s.gids = new Glyph[nl + nr + 1];
	s.gids[0] = nl + nr;
	memcpy(&s.gids[1], extract_glyphptr(ls, lt), nl * sizeof(Glyph));
	memcpy(&s.gids[1 + nl], extract_glyphptr(rs, rt), nr * sizeof(Glyph));
	t = T_GLYPHS;
    } else
	throw Error();
}

void
Substitution::assign_append(Substitute &s, uint8_t &t, const Substitute &ls, uint8_t lt, Glyph rg)
{
    Substitute rs;
    rs.gid = rg;
    assign_append(s, t, ls, lt, rs, T_GLYPH);
}

Substitution
Substitution::in_out_append_glyph(Glyph g) const
{
    Substitution s;
    assign(s._left, s._left_is, _left, _left_is);
    assign(s._right, s._right_is, _right, _right_is);
    assign_append(s._in, s._in_is, _in, _in_is, g);
    assign_append(s._out, s._out_is, _out, _out_is, g);
    return s;
}

void
Substitution::add_outer_left(Glyph g)
{
    Substitute x = _left;
    uint8_t x_is = _left_is;
    _left_is = T_NONE;
    Substitute ls;
    ls.gid = g;
    assign_append(_left, _left_is, ls, T_GLYPH, x, x_is);
    clear(x, x_is);
}

void
Substitution::add_outer_right(Glyph g)
{
    Substitute x = _right;
    uint8_t x_is = _right_is;
    _right_is = T_NONE;
    assign_append(_right, _right_is, x, x_is, g);
    clear(x, x_is);
}

bool
Substitution::out_alter(const Substitution &o, int pos) throw ()
{
    const Glyph *g = out_glyphptr();
    int ng = out_nglyphs();
    const Glyph *out_g = o.out_glyphptr();
    int out_ng = o.out_nglyphs();
    int in_ng = o.in_nglyphs();
    if (pos + in_ng > ng || out_ng == 0)
	return false;
    
    // check that input substitution actually matches us
    for (int i = 0; i < in_ng; i++)
	if (!o.in_matches(i, g[pos+i]))
	    return false;

    // actually change output
    Vector<Glyph> new_g;
    for (int i = 0; i < pos; i++)
	new_g.push_back(g[i]);
    for (int i = 0; i < out_ng; i++)
	new_g.push_back(out_g[i]);
    for (int i = pos + in_ng; i < ng; i++)
	new_g.push_back(g[i]);
    assign(_out, _out_is, new_g.size(), &new_g[0]);
    
    return true;
}

static void
unparse_glyphid(StringAccum &sa, Glyph gid, const Vector<PermString> *gns)
{
    if (gid && gns && gns->size() > gid && (*gns)[gid])
	sa << (*gns)[gid];
    else
	sa << "g" << gid;
}

void
Substitution::unparse(StringAccum &sa, const Vector<PermString> *gns) const
{
    if (!*this)
	sa << "NULL[]";
    else if (is_single()) {
	sa << "SINGLE[";
	unparse_glyphid(sa, _in.gid, gns);
	sa << " => ";
	unparse_glyphid(sa, _out.gid, gns);
	sa << ']';
    } else if (is_ligature()) {
	sa << "LIGATURE[";
	for (int i = 1; i <= _in.gids[0]; i++) {
	    unparse_glyphid(sa, _in.gids[i], gns);
	    sa << ' ';
	}
	sa << "=> ";
	unparse_glyphid(sa, _out.gid, gns);
	sa << ']';
    } else if (is_multiple()) {
	sa << "MULTIPLE[";
	unparse_glyphid(sa, _in.gid, gns);
	sa << " =>";
	for (int i = 1; i <= _out.gids[0]; i++) {
	    sa << ' ';
	    unparse_glyphid(sa, _out.gids[i], gns);
	}
	sa << ']';
    } else if (is_single_rcontext()) {
	sa << "SINGLE_RCONTEXT[";
	unparse_glyphid(sa, _in.gid, gns);
	sa << " => ";
	unparse_glyphid(sa, _out.gid, gns);
	sa << " | ";
	unparse_glyphid(sa, _right.gid, gns);
	sa << ']';
    } else if (is_single_lcontext()) {
	sa << "SINGLE_LCONTEXT[";
	unparse_glyphid(sa, _left.gid, gns);
	sa << " | ";
	unparse_glyphid(sa, _in.gid, gns);
	sa << " => ";
	unparse_glyphid(sa, _out.gid, gns);
	sa << ']';
    } else
	sa << "UNKNOWN[" << (int)_left_is << ',' << (int)_in_is << ',' << (int)_out_is << ',' << (int)_right_is << "]";
}

String
Substitution::unparse(const Vector<PermString> *gns) const
{
    StringAccum sa;
    unparse(sa, gns);
    return sa.take_string();
}



/**************************
 * Gsub                   *
 *                        *
 **************************/

Gsub::Gsub(const Data &d, ErrorHandler *errh) throw (Error)
{
    // Fixed	Version
    // Offset	ScriptList
    // Offset	FeatureList
    // Offset	LookupList
    if (d.u16(0) != 1)
	throw Format("GSUB");
    if (_script_list.assign(d.offset_subtable(4), errh) < 0)
	throw Format("GSUB script list");
    if (_feature_list.assign(d.offset_subtable(6), errh) < 0)
	throw Format("GSUB feature list");
    _lookup_list = d.offset_subtable(8);
}

int
Gsub::nlookups() const
{
    return _lookup_list.u16(0);
}

GsubLookup
Gsub::lookup(unsigned i) const
{
    if (i >= _lookup_list.u16(0))
	throw Error("GSUB lookup out of range");
    else
	return GsubLookup(_lookup_list.offset_subtable(2 + i*2));
}


/**************************
 * GsubLookup             *
 *                        *
 **************************/

GsubLookup::GsubLookup(const Data &d) throw (Error)
    : _d(d)
{
    if (_d.length() < 6)
	throw Format("GSUB Lookup table");
}

bool
GsubLookup::unparse_automatics(const Gsub &gsub, Vector<Substitution> &v) const
{
    int nlookup = _d.u16(4);
    switch (_d.u16(0)) {
      case L_SINGLE:
	for (int i = 0; i < nlookup; i++)
	    GsubSingle(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	return true;
      case L_MULTIPLE:
	for (int i = 0; i < nlookup; i++)
	    GsubMultiple(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	return true;
      case L_ALTERNATE:
	for (int i = 0; i < nlookup; i++)
	    GsubMultiple(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v, true);
	return true;
      case L_LIGATURE:
	for (int i = 0; i < nlookup; i++)
	    GsubLigature(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	return true;
      case L_CONTEXT: {
	  bool understood = true;
	  for (int i = 0; i < nlookup; i++)
	      understood &= GsubContext(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(gsub, v);
	  return understood;
      }
      case L_CHAIN: {
	  bool understood = true;
	  for (int i = 0; i < nlookup; i++)
	      understood &= GsubChainContext(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(gsub, v);
	  return understood;
      }
      default:
	return false;
    }
}

bool
GsubLookup::apply(const Glyph *g, int pos, int n, Substitution &s) const
{
    int nlookup = _d.u16(4);
    switch (_d.u16(0)) {
      case L_SINGLE:
	for (int i = 0; i < nlookup; i++)
	    if (GsubSingle(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).apply(g, pos, n, s))
		return true;
	return false;
      case L_MULTIPLE:
	for (int i = 0; i < nlookup; i++)
	    if (GsubMultiple(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).apply(g, pos, n, s))
		return true;
	return false;
      case L_ALTERNATE:
	for (int i = 0; i < nlookup; i++)
	    if (GsubMultiple(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).apply(g, pos, n, s, true))
		return true;
	return false;
      case L_LIGATURE:
	for (int i = 0; i < nlookup; i++)
	    if (GsubLigature(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).apply(g, pos, n, s))
		return true;
	return false;
      default:			// XXX
	return false;
    }
}


/**************************
 * GsubSingle             *
 *                        *
 **************************/

GsubSingle::GsubSingle(const Data &d) throw (Error)
    : _d(d)
{
    if (_d[0] != 0
	|| (_d[1] != 1 && _d[1] != 2))
	throw Format("GSUB Single Substitution");
    Coverage coverage(_d.offset_subtable(2));
    if (!coverage.ok()
	|| (_d[1] == 2 && coverage.size() > _d.u16(4)))
	throw Format("GSUB Single Substitution coverage");
}

Coverage
GsubSingle::coverage() const throw ()
{
    return Coverage(_d.offset_subtable(2), 0, false);
}

Glyph
GsubSingle::map(Glyph g) const
{
    int ci = coverage().lookup(g);
    if (ci < 0)
	return g;
    else if (_d[1] == 1)
	return g + _d.s16(4);
    else
	return _d.u16(HEADERSIZE + FORMAT2_RECSIZE*ci);
}

void
GsubSingle::unparse(Vector<Substitution> &v) const
{
    if (_d[1] == 1) {
	int delta = _d.s16(4);
	for (Coverage::iterator i = coverage().begin(); i; i++)
	    v.push_back(Substitution(*i, *i + delta));
    } else {
	for (Coverage::iterator i = coverage().begin(); i; i++)
	    v.push_back(Substitution(*i, _d.u16(HEADERSIZE + i.coverage_index()*FORMAT2_RECSIZE)));
    }
}

bool
GsubSingle::apply(const Glyph *g, int pos, int n, Substitution &s) const
{
    int ci;
    if (pos < n	&& (ci = coverage().lookup(g[pos])) >= 0) {
	if (_d[1] == 1)
	    s = Substitution(g[pos], g[pos] + _d.s16(4));
	else
	    s = Substitution(g[pos], _d.u16(HEADERSIZE + ci*FORMAT2_RECSIZE));
	return true;
    } else
	return false;
}


/**************************
 * GsubMultiple           *
 *                        *
 **************************/

GsubMultiple::GsubMultiple(const Data &d) throw (Error)
    : _d(d)
{
    if (_d[0] != 0 || _d[1] != 1)
	throw Format("GSUB Multiple Substitution");
    Coverage coverage(_d.offset_subtable(2));
    if (!coverage.ok()
	|| coverage.size() > _d.u16(4))
	throw Format("GSUB Multiple Substitution coverage");
}

Coverage
GsubMultiple::coverage() const throw ()
{
    return Coverage(_d.offset_subtable(2), 0, false);
}

bool
GsubMultiple::map(Glyph g, Vector<Glyph> &v) const
{
    v.clear();
    int ci = coverage().lookup(g);
    if (ci < 0) {
	v.push_back(g);
	return false;
    } else {
	Data seq = _d.offset_subtable(HEADERSIZE + ci*RECSIZE);
	for (int i = 0; i < seq.u16(0); i++)
	    v.push_back(seq.u16(SEQ_HEADERSIZE + i*SEQ_RECSIZE));
	return true;
    }
}

void
GsubMultiple::unparse(Vector<Substitution> &v, bool is_alternate) const
{
    Vector<Glyph> result;
    for (Coverage::iterator i = coverage().begin(); i; i++) {
	Data seq = _d.offset_subtable(HEADERSIZE + i.coverage_index()*RECSIZE);
	result.clear();
	for (int j = 0; j < seq.u16(0); j++)
	    result.push_back(seq.u16(SEQ_HEADERSIZE + j*SEQ_RECSIZE));
	v.push_back(Substitution(*i, result, is_alternate));
    }
}

bool
GsubMultiple::apply(const Glyph *g, int pos, int n, Substitution &s, bool is_alternate) const
{
    int ci;
    if (pos < n	&& (ci = coverage().lookup(g[pos])) >= 0) {
	Vector<Glyph> result;
	Data seq = _d.offset_subtable(HEADERSIZE + ci*RECSIZE);
	for (int j = 0; j < seq.u16(0); j++)
	    result.push_back(seq.u16(SEQ_HEADERSIZE + j*SEQ_RECSIZE));
	s = Substitution(g[pos], result, is_alternate);
	return true;
    } else
	return false;
}


/**************************
 * GsubLigature           *
 *                        *
 **************************/

GsubLigature::GsubLigature(const Data &d) throw (Error)
    : _d(d)
{
    if (_d[0] != 0
	|| _d[1] != 1)
	throw Format("GSUB Ligature Substitution");
    Coverage coverage(_d.offset_subtable(2));
    if (!coverage.ok()
	|| coverage.size() > _d.u16(4))
	throw Format("GSUB Ligature Substitution coverage");
}

Coverage
GsubLigature::coverage() const throw ()
{
    return Coverage(_d.offset_subtable(2), 0, false);
}

bool
GsubLigature::map(const Vector<Glyph> &gs, Glyph &result, int &consumed) const
{
    assert(gs.size() > 0);
    result = gs[0];
    consumed = 1;
    int ci = coverage().lookup(gs[0]);
    if (ci < 0)
	return false;
    Data ligset = _d.offset_subtable(HEADERSIZE + ci*RECSIZE);
    int nligset = ligset.u16(0);
    for (int i = 0; i < nligset; i++) {
	Data lig = ligset.offset_subtable(SET_HEADERSIZE + i*SET_RECSIZE);
	int nlig = lig.u16(2);
	if (nlig > gs.size() - 1)
	    goto bad;
	for (int j = 0; j < nlig - 1; j++)
	    if (lig.u16(LIG_HEADERSIZE + j*LIG_RECSIZE) != gs[j + 1])
		goto bad;
	result = lig.u16(0);
	consumed = nlig + 1;
	return true;
      bad: ;
    }
    return false;
}

void
GsubLigature::unparse(Vector<Substitution> &v) const
{
    for (Coverage::iterator i = coverage().begin(); i; i++) {
	Data ligset = _d.offset_subtable(HEADERSIZE + i.coverage_index()*RECSIZE);
	int nligset = ligset.u16(0);
	Vector<Glyph> components(1, *i);
	for (int j = 0; j < nligset; j++) {
	    Data lig = ligset.offset_subtable(SET_HEADERSIZE + j*SET_RECSIZE);
	    int nlig = lig.u16(2);
	    components.resize(1);
	    for (int k = 0; k < nlig - 1; k++)
		components.push_back(lig.u16(LIG_HEADERSIZE + k*LIG_RECSIZE));
	    v.push_back(Substitution(components, lig.u16(0)));
	}
    }
}

bool
GsubLigature::apply(const Glyph *g, int pos, int n, Substitution &s) const
{
    int ci;
    if (pos < n	&& (ci = coverage().lookup(g[pos])) >= 0) {
	Data ligset = _d.offset_subtable(HEADERSIZE + ci*RECSIZE);
	int nligset = ligset.u16(0);
	for (int j = 0; j < nligset; j++) {
	    Data lig = ligset.offset_subtable(SET_HEADERSIZE + j*SET_RECSIZE);
	    int nlig = lig.u16(2);
	    if (pos + nlig <= n) {
		for (int k = 0; k < nlig - 1; k++)
		    if (lig.u16(LIG_HEADERSIZE + k*LIG_RECSIZE) != g[pos + k + 1])
			goto ligature_failed;
		s = Substitution(nlig, &g[pos], lig.u16(0));
		return true;
	    }
	  ligature_failed: ;
	}
    }
    return false;
}


/**************************
 * GsubContext            *
 *                        *
 **************************/

GsubContext::GsubContext(const Data &d) throw (Error)
    : _d(d)
{
    switch (_d.u16(0)) {
      case 1:
      case 2:
	break;
      case 3: {
	  int ninput = _d.u16(2);
	  if (ninput < 1)
	      throw Format("GSUB Context Substitution input sequence");
	  Coverage coverage(_d.offset_subtable(F3_HSIZE));
	  if (!coverage.ok())
	      throw Format("GSUB Context Substitution coverage");
	  break;
      }
      default:
	throw Format("GSUB Context Substitution");
    }
}

Coverage
GsubContext::coverage() const throw ()
{
    if (_d[1] == 3)
	return Coverage(_d.offset_subtable(F3_HSIZE), 0, false);
    else
	return Coverage();
}

bool
GsubContext::f3_unparse(const Data &data, int nglyph, int glyphtab_offset, int nsub, int subtab_offset, const Gsub &gsub, Vector<Substitution> &outsubs, const Substitution &prototype_sub)
{
    Vector<Substitution> subs;
    subs.push_back(prototype_sub);
    Vector<Substitution> work_subs;

    // get array of possible substitutions including contexts
    for (int i = 0; i < nglyph; i++) {
	assert(!work_subs.size());
	Coverage c(data.offset_subtable(glyphtab_offset + i*2));
	for (Coverage::iterator ci = c.begin(); ci; ci++)
	    for (int j = 0; j < subs.size(); j++)
		work_subs.push_back(subs[j].in_out_append_glyph(*ci));
	subs.swap(work_subs);
    }

    // now, apply referred lookups to the resulting substitution array
    Vector<Glyph> in_glyphs;
    Vector<int> pos_map;
    Substitution subtab_sub;
    for (int i = 0; i < subs.size(); i++) {
	Substitution &s = subs[i];
	int napplied = 0;
	for (int j = 0; j < nsub; j++) {
	    int seq_index = data.u16(subtab_offset + SUBRECSIZE*j);
	    int lookup_index = data.u16(subtab_offset + SUBRECSIZE*j + 2);
	    // XXX check seq_index against size of output glyphs?
	    if (gsub.lookup(lookup_index).apply(s.out_glyphptr(), seq_index, s.out_nglyphs(), subtab_sub)) {
		napplied++;
		s.out_alter(subtab_sub, seq_index);
	    }
	}
	// 26.Jun.2003 -- always push substitution back, since the no-op might
	// override a following substitution
	outsubs.push_back(s);
    }

    return true;		// XXX
}

bool
GsubContext::unparse(const Gsub &gsub, Vector<Substitution> &v) const
{
    if (_d.u16(0) != 3)		// XXX
	return false;
    int nglyph = _d.u16(2);
    int nsubst = _d.u16(4);
    return f3_unparse(_d, nglyph, F3_HSIZE, nsubst, F3_HSIZE + nglyph*2, gsub, v, Substitution());
}


/**************************
 * GsubChainContext       *
 *                        *
 **************************/

GsubChainContext::GsubChainContext(const Data &d) throw (Error)
    : _d(d)
{
    switch (_d.u16(0)) {
      case 1:
      case 2:
	break;
      case 3: {
	  int nbacktrack = _d.u16(2);
	  int input_offset = F3_HSIZE + nbacktrack*2;
	  int ninput = _d.u16(input_offset);
	  if (ninput < 1)
	      throw Format("GSUB ChainContext Substitution input sequence");
	  Coverage coverage(_d.offset_subtable(input_offset + F3_INPUT_HSIZE));
	  if (!coverage.ok())
	      throw Format("GSUB ChainContext Substitution coverage");
	  break;
      }
      default:
	throw Format("GSUB ChainContext Substitution");
    }
}

Coverage
GsubChainContext::coverage() const throw ()
{
    if (_d.u16(0) != 3)
	return Coverage();
    int nbacktrack = _d.u16(2);
    int input_offset = F3_HSIZE + nbacktrack*2;
    return Coverage(_d.offset_subtable(input_offset + F3_INPUT_HSIZE), 0, false);
}

bool
GsubChainContext::unparse(const Gsub &gsub, Vector<Substitution> &v) const
{
    if (_d.u16(0) != 3)
	return false;
    
    int nbacktrack = _d.u16(2);
    int input_offset = F3_HSIZE + nbacktrack*2;
    int ninput = _d.u16(input_offset);
    int lookahead_offset = input_offset + F3_INPUT_HSIZE + ninput*2;
    int nlookahead = _d.u16(lookahead_offset);
    int subst_offset = lookahead_offset + F3_LOOKAHEAD_HSIZE + nlookahead*2;
    int nsubst = _d.u16(subst_offset);

    if (nbacktrack == 0 && nlookahead == 0)
	return GsubContext::f3_unparse(_d, ninput, input_offset + F3_INPUT_HSIZE, nsubst, subst_offset + F3_SUBST_HSIZE, gsub, v, Substitution());
    else if (nbacktrack == 0 && ninput == 1 && nlookahead == 1) {
	Coverage c(_d.offset_subtable(lookahead_offset + F3_LOOKAHEAD_HSIZE));
	bool any = false;
	for (Coverage::iterator ci = c.begin(); ci; ci++)
	    any |= GsubContext::f3_unparse(_d, ninput, input_offset + F3_INPUT_HSIZE, nsubst, subst_offset + F3_SUBST_HSIZE, gsub, v, Substitution(Substitution::C_RIGHT, *ci));
	return any;
    } else if (nbacktrack == 1 && ninput == 1 && nlookahead == 0) {
	Coverage c(_d.offset_subtable(F3_HSIZE));
	bool any = false;
	for (Coverage::iterator ci = c.begin(); ci; ci++)
	    any |= GsubContext::f3_unparse(_d, ninput, input_offset + F3_INPUT_HSIZE, nsubst, subst_offset + F3_SUBST_HSIZE, gsub, v, Substitution(Substitution::C_LEFT, *ci));
	return any;
    } else
	return false;
}


}}
