// -*- related-file-name: "../include/efont/otfgsub.hh" -*-
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
Substitution::extract_glyph(const Substitute &s, uint8_t t)
{
    return (t == T_GLYPH ? s.gid : 0);
}

bool
Substitution::extract_glyphs(const Substitute &s, uint8_t t, Vector<Glyph> &v)
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
    } else
	sa << "UNKNOWN[]";
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

void
GsubLookup::unparse_automatics(Vector<Substitution> &v) const
{
    int n = _d.u16(4);
    switch (_d.u16(0)) {
      case L_SINGLE:
	for (int i = 0; i < n; i++)
	    GsubSingle(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	break;
      case L_MULTIPLE:
	for (int i = 0; i < n; i++)
	    GsubMultiple(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	break;
      case L_ALTERNATE:
	for (int i = 0; i < n; i++)
	    GsubMultiple(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v, true);
	break;
      case L_LIGATURE:
	for (int i = 0; i < n; i++)
	    GsubLigature(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	break;
      default:
	/* XXX */
	break;
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


}}

#include <lcdf/vector.cc>
