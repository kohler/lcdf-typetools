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
#include <netinet/in.h>		// for ntohl()

#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))
#define SHORT_AT(d)		((int16_t) ntohs(*(const uint16_t *)(d)))
#define ULONG_AT(d)		(ntohl(*(const uint32_t *)(d)))

#define USHORT_ATX(d)		(((uint8_t)*(d) << 8) | (uint8_t)*((d)+1))
#define ULONG_ATX(d)		((USHORT_ATX((d)) << 16) | USHORT_ATX((d)+2))

namespace Efont {

void
OpenTypeSubstitution::clear(Substitute &s, uint8_t &t)
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
OpenTypeSubstitution::assign(Substitute &s, uint8_t &t, OpenTypeGlyph gid)
{
    clear(s, t);
    s.gid = gid;
    t = T_GLYPH;
}

void
OpenTypeSubstitution::assign(Substitute &s, uint8_t &t, int ngids, const OpenTypeGlyph *gids)
{
    clear(s, t);
    assert(ngids > 0);
    if (ngids == 1) {
	s.gid = gids[0];
	t = T_GLYPH;
    } else {
	s.gids = new OpenTypeGlyph[ngids + 1];
	s.gids[0] = ngids;
	memcpy(s.gids + 1, gids, ngids * sizeof(OpenTypeGlyph));
	t = T_GLYPHS;
    }
}

void
OpenTypeSubstitution::assign(Substitute &s, uint8_t &t, const OpenTypeCoverage &coverage)
{
    clear(s, t);
    s.coverage = new OpenTypeCoverage(coverage);
    t = T_COVERAGE;
}

void
OpenTypeSubstitution::assign(Substitute &s, uint8_t &t, const Substitute &os, uint8_t ot)
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

OpenTypeSubstitution::OpenTypeSubstitution(const OpenTypeSubstitution &o)
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_NONE), _right_is(T_NONE)
{
    assign(_left, _left_is, o._left, o._left_is);
    assign(_in, _in_is, o._in, o._in_is);
    assign(_out, _out_is, o._out, o._out_is);
    assign(_right, _right_is, o._right, o._right_is);
}

OpenTypeSubstitution::OpenTypeSubstitution(OpenTypeGlyph in, OpenTypeGlyph out)
    : _left_is(T_NONE), _in_is(T_GLYPH), _out_is(T_GLYPH), _right_is(T_NONE)
{
    _in.gid = in;
    _out.gid = out;
}

OpenTypeSubstitution::OpenTypeSubstitution(OpenTypeGlyph in1, OpenTypeGlyph in2, OpenTypeGlyph out)
    : _left_is(T_NONE), _in_is(T_GLYPHS), _out_is(T_GLYPH), _right_is(T_NONE)
{
    _in.gids = new OpenTypeGlyph[3];
    _in.gids[0] = 2;
    _in.gids[1] = in1;
    _in.gids[2] = in2;
    _out.gid = out;
}

OpenTypeSubstitution::OpenTypeSubstitution(const Vector<OpenTypeGlyph> &in, OpenTypeGlyph out)
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_GLYPH), _right_is(T_NONE)
{
    assert(in.size() > 0);
    assign(_in, _in_is, in.size(), &in[0]);
    _out.gid = out;
}

OpenTypeSubstitution::~OpenTypeSubstitution()
{
    clear(_left, _left_is);
    clear(_in, _in_is);
    clear(_out, _out_is);
    clear(_right, _right_is);
}

OpenTypeSubstitution &
OpenTypeSubstitution::operator=(const OpenTypeSubstitution &o)
{
    if (&o != this) {
	assign(_left, _left_is, o._left, o._left_is);
	assign(_in, _in_is, o._in, o._in_is);
	assign(_out, _out_is, o._out, o._out_is);
	assign(_right, _right_is, o._right, o._right_is);
    }
    return *this;
}

static void
unparse_glyphid(StringAccum &sa, OpenTypeGlyph gid, const Vector<PermString> *gns)
{
    if (gid && gns && gns->size() > gid && (*gns)[gid])
	sa << (*gns)[gid];
    else
	sa << "g" << gid;
}

void
OpenTypeSubstitution::unparse(StringAccum &sa, const Vector<PermString> *gns) const
{
    if (_left_is == T_NONE && _in_is == T_NONE && _out_is == T_NONE && _right_is == T_NONE)
	sa << "NULL[]";
    else if (_left_is == T_NONE && _in_is == T_GLYPH && _out_is == T_GLYPH && _right_is == T_NONE) {
	sa << "SINGLE[";
	unparse_glyphid(sa, _in.gid, gns);
	sa << " => ";
	unparse_glyphid(sa, _out.gid, gns);
	sa << ']';
    } else if (_left_is == T_NONE && _in_is == T_GLYPHS && _out_is == T_GLYPH && _right_is == T_NONE) {
	sa << "LIGATURE[";
	for (int i = 1; i <= _in.gids[0]; i++) {
	    unparse_glyphid(sa, _in.gids[i], gns);
	    sa << ' ';
	}
	sa << "=> ";
	unparse_glyphid(sa, _out.gid, gns);
	sa << ']';
    } else if (_left_is == T_NONE && _in_is == T_GLYPH && _out_is == T_GLYPHS && _right_is == T_NONE) {
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
OpenTypeSubstitution::unparse(const Vector<PermString> *gns) const
{
    StringAccum sa;
    unparse(sa, gns);
    return sa.take_string();
}



/**************************
 * OpenType_GSUB          *
 *                        *
 **************************/

OpenType_GSUB::OpenType_GSUB(const String &str, ErrorHandler *errh)
    : _str(str)
{
    _str.align(4);
    if (check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	_str = String();
}

int
OpenType_GSUB::check(ErrorHandler *errh)
{
    // HEADER FORMAT:
    // Fixed	Version
    // Offset	ScriptList
    // Offset	FeatureList
    // Offset	LookupList
    int len = _str.length();
    const uint8_t *data = _str.udata();
    if (len < HEADERSIZE)
	return errh->error("OTF GSUB too small for header"), -EFAULT;
    if (!(data[0] == '\000' && data[1] == '\001'))
	return errh->error("OTF GSUB bad version number"), -ERANGE;

    if (_script_list.assign(_str.substring(USHORT_AT(data + 4)), errh) < 0)
	return -1;
    if (_feature_list.assign(_str.substring(USHORT_AT(data + 6)), errh) < 0)
	return -1;
    return 0;
}


/**************************
 * OpenType_GSUBSingle    *
 *                        *
 **************************/

OpenType_GSUBSingle::OpenType_GSUBSingle(const String &str, ErrorHandler *errh)
    : _str(str)
{
    _str.align(2);
    if (check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	_str = String();
}

int
OpenType_GSUBSingle::check(ErrorHandler *errh)
{
    const uint8_t *data = _str.udata();
    if (_str.length() < HEADERSIZE
	|| data[0] != 0
	|| (data[1] != 1 && data[1] != 2)
	|| (data[1] == 2 && _str.length() < HEADERSIZE + USHORT_AT(data + 4)*FORMAT2_RECSIZE))
	return errh->error("GSUB Single Substitution table bad format");
    OpenTypeCoverage coverage(_str.substring(USHORT_AT(data + 2)), errh);
    if (!coverage.ok())
	return -1;
    if (data[1] == 2 && coverage.size() > USHORT_AT(data + 4))
	return errh->error("GSUB Single Substitution Format 2 coverage mismatch");
    return 0;
}

OpenTypeCoverage
OpenType_GSUBSingle::coverage() const
{
    if (!ok())
	return OpenTypeCoverage();
    else {
	const uint8_t *data = _str.udata();
	return OpenTypeCoverage(_str.substring(USHORT_AT(data + 2)), 0, false);
    }
}

bool
OpenType_GSUBSingle::covers(OpenTypeGlyph g) const
{
    if (!ok())
	return false;
    else {
	const uint8_t *data = _str.udata();
	return OpenTypeCoverage(_str.substring(USHORT_AT(data + 2)), 0, false).covers(g);
    }
}

OpenTypeGlyph
OpenType_GSUBSingle::map(OpenTypeGlyph g) const
{
    if (!ok())
	return g;
    const uint8_t *data = _str.udata();
    int ci = OpenTypeCoverage(_str.substring(USHORT_AT(data + 2)), 0, false).lookup(g);
    if (ci < 0)
	return g;
    else if (data[1] == 1)
	return g + SHORT_AT(data + 4);
    else
	return USHORT_AT(data + HEADERSIZE + FORMAT2_RECSIZE*ci);
}


/**************************
 * OpenType_GSUBLigature  *
 *                        *
 **************************/

OpenType_GSUBLigature::OpenType_GSUBLigature(const String &str, ErrorHandler *errh)
    : _str(str)
{
    _str.align(2);
    if (check(errh ? errh : ErrorHandler::silent_handler()) < 0)
	_str = String();
}

int
OpenType_GSUBLigature::check(ErrorHandler *errh)
{
    const uint8_t *data = _str.udata();
    if (_str.length() < HEADERSIZE
	|| data[0] != 0
	|| data[1] != 1
	|| _str.length() < HEADERSIZE + USHORT_AT(data + 4)*RECSIZE)
	return errh->error("GSUB Ligature Substitution table bad format");
    OpenTypeCoverage coverage(_str.substring(USHORT_AT(data + 2)), errh);
    if (!coverage.ok())
	return -1;
    if (coverage.size() > USHORT_AT(data + 4))
	return errh->error("GSUB Ligature Substitution coverage mismatch");
    return 0;
}

OpenTypeCoverage
OpenType_GSUBLigature::coverage() const
{
    if (!ok())
	return OpenTypeCoverage();
    else {
	const uint8_t *data = _str.udata();
	return OpenTypeCoverage(_str.substring(USHORT_AT(data + 2)), 0, false);
    }
}

bool
OpenType_GSUBLigature::covers(OpenTypeGlyph g) const
{
    if (!ok())
	return false;
    else {
	const uint8_t *data = _str.udata();
	return OpenTypeCoverage(_str.substring(USHORT_AT(data + 2)), 0, false).covers(g);
    }
}

bool
OpenType_GSUBLigature::map(const Vector<OpenTypeGlyph> &gs,
			   OpenTypeGlyph &result, int &consumed) const
{
    assert(gs.size() > 0);
    result = gs[0];
    consumed = 1;
    if (!ok())
	return false;
    const uint8_t *data = _str.udata();
    int ci = OpenTypeCoverage(_str.substring(USHORT_AT(data + 2)), 0, false).lookup(gs[0]);
    if (ci < 0)
	return false;
    int off = USHORT_AT(data + HEADERSIZE + RECSIZE*ci), last_off;
    if (off + SET_HEADERSIZE > _str.length()
	|| ((last_off = off + SET_HEADERSIZE + USHORT_AT(data + off)*SET_RECSIZE),
	    last_off > _str.length()))
	// XXX errh->error("GSUB Ligature Substitution table bad for glyph %d", gs[0]);
	return false;
    for (int i = off + SET_HEADERSIZE; i < last_off; i += SET_RECSIZE) {
	int off2 = off + USHORT_AT(i), ninlig;
	if (off2 + LIG_HEADERSIZE > _str.length()
	    || ((ninlig = USHORT_AT(data + off2 + 2)),
		off2 + LIG_HEADERSIZE + ninlig*LIG_RECSIZE > _str.length()))
	    // XXX errh->error("GSUB Ligature Substitution ligature bad for glyph %d", gs[0]);
	    return false;
	if (ninlig > gs.size() - 1)
	    goto bad;
	for (int j = 0; j < ninlig; j++)
	    if (USHORT_AT(data + off2 + LIG_HEADERSIZE + j*LIG_RECSIZE) != gs[j + 1])
		goto bad;
	result = USHORT_AT(data + off2);
	consumed = ninlig + 1;
	return true;
      bad: ;
    }
    return false;
}

}
