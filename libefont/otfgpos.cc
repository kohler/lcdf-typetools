// -*- related-file-name: "../include/efont/otfgpos.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/otfgpos.hh>
#include <lcdf/error.hh>
#include <lcdf/straccum.hh>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace Efont { namespace OpenType {


/**************************
 * Gpos                   *
 *                        *
 **************************/

Gpos::Gpos(const Data &d, ErrorHandler *errh) throw (Error)
{
    // Fixed	Version
    // Offset	ScriptList
    // Offset	FeatureList
    // Offset	LookupList
    if (d.u16(0) != 1)
	throw Format("GPOS");
    if (_script_list.assign(d.offset_subtable(4), errh) < 0)
	throw Format("GPOS script list");
    if (_feature_list.assign(d.offset_subtable(6), errh) < 0)
	throw Format("GPOS feature list");
    _lookup_list = d.offset_subtable(8);
}

GposLookup
Gpos::lookup(unsigned i) const
{
    if (i >= _lookup_list.u16(0))
	throw Error("GPOS lookup out of range");
    else
	return GposLookup(_lookup_list.offset_subtable(2 + i*2));
}


/**************************
 * GposValue              *
 *                        *
 **************************/

const int GposValue::nibble_bitcount_x2[] = { 0, 2, 2, 4, 2, 4, 4, 6,
					      2, 4, 4, 6, 4, 6, 6, 8 };


/**************************
 * GposLookup             *
 *                        *
 **************************/

GposLookup::GposLookup(const Data &d) throw (Error)
    : _d(d)
{
    if (_d.length() < 6)
	throw Format("GPOS Lookup table");
}

void
GposLookup::unparse_automatics(Vector<Positioning> &v) const
{
    int n = _d.u16(4);
    switch (_d.u16(0)) {
      case L_SINGLE:
	for (int i = 0; i < n; i++)
	    GposSingle(_d.offset_subtable(HEADERSIZE + i*RECSIZE)).unparse(v);
	break;
      default:
	/* XXX */
	break;
    }
}


/**************************
 * GposSingle             *
 *                        *
 **************************/

GposSingle::GposSingle(const Data &d) throw (Error)
    : _d(d)
{
    if (_d[0] != 0
	|| (_d[1] != 1 && _d[1] != 2))
	throw Format("GPOS Single Adjustment");
    Coverage coverage(_d.offset_subtable(2));
    if (!coverage.ok()
	|| (_d[1] == 2 && coverage.size() > _d.u16(6)))
	throw Format("GPOS Single Adjustment coverage");
}

Coverage
GposSingle::coverage() const throw ()
{
    return Coverage(_d.offset_subtable(2), 0, false);
}

void
GposSingle::unparse(Vector<Positioning> &v) const
{
    if (_d[1] == 1) {
	int format = _d.u16(4);
	Data value = _d.subtable(6);
	for (Coverage::iterator i = coverage().begin(); i; i++)
	    v.push_back(Positioning(Position(*i, format, value)));
    } else {
	int format = _d.u16(4);
	int size = GposValue::size(format);
	for (Coverage::iterator i = coverage().begin(); i; i++)
	    v.push_back(Positioning(Position(*i, format, _d.subtable(8 + size*i.coverage_index()))));
    }
}


/**************************
 * Positioning            *
 *                        *
 **************************/

bool
Positioning::context_in(const Coverage &c) const
{
    return (c.covers(_left.g) || !_left.g) && (!_right.g || c.covers(_right.g));
}

bool
Positioning::context_in(const GlyphSet &gs) const
{
    return (gs.covers(_left.g) || !_left.g) && (!_right.g || gs.covers(_right.g));
}

static void
unparse_glyphid(StringAccum &sa, Glyph gid, const Vector<PermString> *gns)
{
    if (gid && gns && gns->size() > gid && (*gns)[gid])
	sa << (*gns)[gid];
    else
	sa << "g" << gid;
}

static void
unparse_position(StringAccum &sa, const Position &pos, const Vector<PermString> *gns)
{
    unparse_glyphid(sa, pos.g, gns);
    if (pos.placed())
	sa << '@' << pos.pdx << ',' << pos.pdy;
    sa << '+' << pos.adx;
    if (pos.ady)
	sa << '/' << pos.ady;
}

void
Positioning::unparse(StringAccum &sa, const Vector<PermString> *gns) const
{
    if (!*this)
	sa << "NULL[]";
    else if (is_single()) {
	sa << "SINGLE[";
	unparse_position(sa, _left, gns);
	sa << ']';
    } else if (is_pairkern()) {
	sa << "KERN[";
	unparse_glyphid(sa, _left.g, gns);
	sa << ' ';
	unparse_glyphid(sa, _right.g, gns);
	sa << "+" << _left.adx << ']';
    } else if (is_pair()) {
	sa << "PAIR[";
	unparse_position(sa, _left, gns);
	sa << ' ';
	unparse_position(sa, _right, gns);
	sa << ']';
    } else
	sa << "UNKNOWN[]";
}

String
Positioning::unparse(const Vector<PermString> *gns) const
{
    StringAccum sa;
    unparse(sa, gns);
    return sa.take_string();
}

}}

#include <lcdf/vector.cc>
