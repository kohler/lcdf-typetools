/* maket1font.{cc,hh} -- translate CFF fonts to Type 1 fonts
 *
 * Copyright (c) 2002-2003 Eddie Kohler
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
#include "maket1font.hh"
#include <efont/t1interp.hh>
#include <efont/t1csgen.hh>
#include <lcdf/point.hh>
#include <lcdf/error.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <efont/t1unparser.hh>

using namespace Efont;

typedef unsigned CsRef;
enum { CSR_GLYPH = 0x00000000, CSR_SUBR = 0x80000000,
       CSR_GSUBR = 0xC0000000,
       CSR_TYPE = 0xC0000000, CSR_NUM = 0x3FFFFFFF };

class MakeType1CharstringInterp : public CharstringInterp { public:

    MakeType1CharstringInterp(int precision = 5);
    ~MakeType1CharstringInterp();

    Type1Font *output() const			{ return _output; }
    
    void run(const CharstringProgram *, Type1Font *, PermString glyph_definer, ErrorHandler *);
    void run(const CharstringContext &, Type1Charstring &, ErrorHandler *);

    bool type2_command(int, const uint8_t *, int *);
    
    void act_sidebearing(int, const Point &);
    void act_width(int, const Point &);
    void act_seac(int, double, double, double, int, int);

    void act_hstem(int, double, double);
    void act_vstem(int, double, double);
    void act_hintmask(int, const unsigned char *, int);

    void act_line(int, const Point &, const Point &);
    void act_curve(int, const Point &, const Point &, const Point &, const Point &);
    void act_closepath(int);
    virtual void act_flex(int, const Point &, const Point &, const Point &, const Point &, const Point &, const Point &, const Point &, double);

    int nhints() const			{ return _stem_pos.size(); }
    double max_flex_height() const	{ return _max_flex_height; }
    String landmark() const;

    class Subr;
    
  private:

    // output
    Type1CharstringGen _csgen;
    Type1Font *_output;
    ErrorHandler *_errh;

    // current glyph
    Point _sidebearing;
    Point _width;
    enum State { S_INITIAL, S_OPEN, S_CLOSED, S_SEAC };
    State _state;

    Vector<double> _stem_pos;
    Vector<double> _stem_width;
    int _nhstem;

    // hint replacement
    Type1CharstringGen _hr_csgen;
    int _hr_firstsubr;

    // Flex
    double _max_flex_height;
    bool _flex_message;

    // subroutines
    int _subr_bias;
    int _gsubr_bias;
    mutable Vector<Subr *> _glyphs;
    mutable Vector<Subr *> _subrs;
    mutable Vector<Subr *> _gsubrs;

    Subr *_cur_subr;
    int _cur_glyph;

    void gen_number(double, int = 0);
    void gen_command(int);
    void gen_sbw(bool hints_follow);
    void gen_hintmask(Type1CharstringGen &, const unsigned char *, int) const;

    Subr *csr_subr(CsRef, bool force) const;
    Type1Charstring *csr_charstring(CsRef) const;
    
};

class MakeType1CharstringInterp::Subr { public:

    Subr(CsRef csr)			: _csr(csr), _output_subrno(-1), _stamp(0) { }

    bool up_to_date() const		{ return _stamp == max_stamp; }
    void update()			{ _stamp = max_stamp; }
    static void bump_date()		{ max_stamp++; }

    Type1Charstring *charstring(const MakeType1CharstringInterp *) const;
    
    int ncalls() const			{ return _calls.size(); }
    Subr *call(int i) const		{ return _calls[i]; }
    bool has_call(Subr *) const;

    struct Caller {
	Subr *subr;
	int pos;
	int len;
	String s;
	Caller(Subr *s, int p, int l)	: subr(s), pos(p), len(l) { }
	String charstring(MakeType1CharstringInterp *mcsi) const { return subr->charstring(mcsi)->substring(pos, len); }
    };

    int ncallers() const		{ return _callers.size(); }
    const Caller &caller(int i) const	{ return _callers[i]; }
    
    void add_call(Subr *s)		{ _calls.push_back(s); }
    void add_caller(Subr *, int, int);

    int output_subrno() const		{ return _output_subrno; }
    void set_output_subrno(int n)	{ _output_subrno = n; }

    void transfer_nested_calls(int pos, int length, Subr *new_caller) const;
    void change_callers(Subr *, int pos, int length, int new_length);
    bool unify(MakeType1CharstringInterp *);
    
    void caller_data(bool assign, MakeType1CharstringInterp *mcsi);
    
  private:

    CsRef _csr;
    Vector<Subr *> _calls;
    Vector<Caller> _callers;

    int _output_subrno;
    int _stamp;

    static int max_stamp;

    friend class MakeType1CharstringInterp;
    
};

int MakeType1CharstringInterp::Subr::max_stamp = 1;

inline void
MakeType1CharstringInterp::Subr::add_caller(Subr *s, int pos, int len)
{
    _callers.push_back(Caller(s, pos, len));
}

bool
MakeType1CharstringInterp::Subr::has_call(Subr *s) const
{
    for (int i = 0; i < _calls.size(); i++)
	if (_calls[i] == s)
	    return true;
    return false;
}


/*****
 * MakeType1CharstringInterp
 **/

MakeType1CharstringInterp::MakeType1CharstringInterp(int precision)
    : _csgen(precision), _errh(0),
      _hr_csgen(precision), _hr_firstsubr(-1), _max_flex_height(0),
      _flex_message(0), _cur_glyph(-1)
{
}

MakeType1CharstringInterp::~MakeType1CharstringInterp()
{
    for (int i = 0; i < _glyphs.size(); i++)
	delete _glyphs[i];
    for (int i = 0; i < _subrs.size(); i++)
	delete _subrs[i];
    for (int i = 0; i < _gsubrs.size(); i++)
	delete _gsubrs[i];
}

String
MakeType1CharstringInterp::landmark() const
{
    if (_cur_glyph >= 0 && _cur_glyph < program()->nglyphs())
	return String("glyph '") + program()->glyph_name(_cur_glyph) + "'";
    else
	return String();
}


// generating charstring commands

inline void
MakeType1CharstringInterp::gen_number(double n, int what)
{
    _csgen.gen_number(n, what);
}

inline void
MakeType1CharstringInterp::gen_command(int what)
{
    _csgen.gen_command(what);
}

void
MakeType1CharstringInterp::gen_sbw(bool hints_follow)
{
    if (!hints_follow && nhints()) {
	String s = String::fill_string('\377', ((nhints() - 1) >> 3) + 1);
	act_hintmask(CS::cHintmask, reinterpret_cast<const unsigned char *>(s.data()), nhints());
    } else if (_sidebearing.y == 0 && _width.y == 0) {
	gen_number(_sidebearing.x);
	gen_number(_width.x);
	gen_command(CS::cHsbw);
    } else {
	gen_number(_sidebearing.x);
	gen_number(_sidebearing.y);
	gen_number(_width.x);
	gen_number(_width.y);
	gen_command(CS::cSbw);
    }
    _state = S_CLOSED;
}

void
MakeType1CharstringInterp::act_sidebearing(int, const Point &p)
{
    _sidebearing = p;
}

void
MakeType1CharstringInterp::act_width(int, const Point &p)
{
    _width = p;
}

void
MakeType1CharstringInterp::act_seac(int, double asb, double adx, double ady, int bchar, int achar)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    gen_number(asb);
    gen_number(adx);
    gen_number(ady);
    gen_number(bchar);
    gen_number(achar);
    gen_command(CS::cSeac);
    _state = S_SEAC;
}

void
MakeType1CharstringInterp::act_hstem(int, double pos, double width)
{
    if (_nhstem == _stem_pos.size()) {
	_stem_pos.push_back(pos);
	_stem_width.push_back(width);
	_nhstem++;
    }
}

void
MakeType1CharstringInterp::act_vstem(int, double pos, double width)
{
    _stem_pos.push_back(pos);
    _stem_width.push_back(width);
}

void
MakeType1CharstringInterp::gen_hintmask(Type1CharstringGen &csgen, const unsigned char *data, int nhints) const
{
    unsigned char mask = 0x80;
    for (int i = 0; i < nhints; i++) {
	if (*data & mask) {
	    csgen.gen_number(_stem_pos[i]);
	    csgen.gen_number(_stem_width[i]);
	    csgen.gen_command(i < _nhstem ? CS::cHstem : CS::cVstem);
	}
	if ((mask >>= 1) == 0)
	    data++, mask = 0x80;
    }
}

void
MakeType1CharstringInterp::act_hintmask(int cmd, const unsigned char *data, int nhints)
{
    if (cmd == CS::cCntrmask || nhints > MakeType1CharstringInterp::nhints())
	return;

    if (_state == S_INITIAL) {
	gen_sbw(true);
	gen_hintmask(_csgen, data, nhints);
    } else if (_hr_firstsubr >= 0) {
	_hr_csgen.clear();
	gen_hintmask(_hr_csgen, data, nhints);
	_hr_csgen.gen_command(CS::cReturn);
	Type1Charstring hr_subr;
	_hr_csgen.output(hr_subr);

	int subrno = -1, nsubrs = _output->nsubrs();
	for (int i = _hr_firstsubr; i < nsubrs; i++)
	    if (Type1Subr *s = _output->subr_x(i))
		if (s->t1cs() == hr_subr) {
		    subrno = i;
		    break;
		}
	
	if (subrno < 0 && _output->set_subr(nsubrs, hr_subr))
	    subrno = nsubrs;

	if (subrno >= 0) {
	    _csgen.gen_number(subrno);
	    _csgen.gen_number(4);
	    _csgen.gen_command(CS::cCallsubr);
	}
    }
}

void
MakeType1CharstringInterp::act_line(int, const Point &a, const Point &b)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    _csgen.gen_moveto(a, _state == S_OPEN);
    _state = S_OPEN;
    if (a.x == b.x) {
	gen_number(b.y - a.y, 'y');
	gen_command(CS::cVlineto);
    } else if (a.y == b.y) {
	gen_number(b.x - a.x, 'x');
	gen_command(CS::cHlineto);
    } else {
	gen_number(b.x - a.x, 'x');
	gen_number(b.y - a.y, 'y');
	gen_command(CS::cRlineto);
    }
}

void
MakeType1CharstringInterp::act_curve(int, const Point &a, const Point &b, const Point &c, const Point &d)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    _csgen.gen_moveto(a, _state == S_OPEN);
    _state = S_OPEN;
    if (b.y == a.y && d.x == c.x) {
	gen_number(b.x - a.x, 'x');
	gen_number(c.x - b.x, 'x');
	gen_number(c.y - b.y, 'y');
	gen_number(d.y - c.y, 'y');
	gen_command(CS::cHvcurveto);
    } else if (b.x == a.x && d.y == c.y) {
	gen_number(b.y - a.y, 'y');
	gen_number(c.x - a.x, 'x');
	gen_number(c.y - b.y, 'y');
	gen_number(d.x - c.x, 'x');
	gen_command(CS::cVhcurveto);
    } else {
	gen_number(b.x - a.x, 'x');
	gen_number(b.y - a.y, 'y');
	gen_number(c.x - b.x, 'x');
	gen_number(c.y - b.y, 'y');
	gen_number(d.x - c.x, 'x');
	gen_number(d.y - c.y, 'y');
	gen_command(CS::cRrcurveto);
    }
}

void
MakeType1CharstringInterp::act_flex(int cmd, const Point &p0, const Point &p1, const Point &p2, const Point &p3_4, const Point &p5, const Point &p6, const Point &p7, double flex_depth)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    _csgen.gen_moveto(p0, _state == S_OPEN);
    _state = S_OPEN;

    // 1. Outer endpoints must have same x (or y) coordinate
    bool v_ok = (p0.x == p7.x);
    bool h_ok = (p0.y == p7.y);
    
    // 2. Join point and its neighboring controls must be at an extreme
    if (v_ok && p2.x == p3_4.x && p3_4.x == p5.x) {
	double distance = fabs(p3_4.x - p0.x);
	int sign = (p3_4.x < p0.x ? -1 : 1);
	if (sign * (p1.x - p0.x) < 0 || sign * (p1.x - p0.x) > distance
	    || sign * (p6.x - p0.x) < 0 || sign * (p6.x - p0.x) > distance)
	    v_ok = false;
    } else
	v_ok = false;

    if (h_ok && p2.y == p3_4.y && p3_4.y == p5.y) {
	double distance = fabs(p3_4.y - p0.y);
	int sign = (p3_4.y < p0.y ? -1 : 1);
	if (sign * (p1.y - p0.y) < 0 || sign * (p1.y - p0.y) > distance
	    || sign * (p6.y - p0.y) < 0 || sign * (p6.y - p0.y) > distance)
	    h_ok = false;
    } else
	h_ok = false;

    // 3. Flex height <= 20
    if (v_ok && fabs(p3_4.x - p0.x) > 20)
	v_ok = false;
    if (h_ok && fabs(p3_4.y - p0.y) > 20)
	h_ok = false;

    // generate flex commands
    if (v_ok || h_ok) {
	Point p_reference = (h_ok ? Point(p3_4.x, p0.y) : Point(p0.x, p3_4.y));

	_csgen.gen_number(1);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p_reference, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p1, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p2, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p3_4, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p5, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p6, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_moveto(p7, false);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_number(flex_depth);
	_csgen.gen_number(p7.x, 'X');
	_csgen.gen_number(p7.y, 'Y');
	_csgen.gen_number(0);
	_csgen.gen_command(CS::cCallsubr);

	double flex_height = fabs(h_ok ? p3_4.y - p0.y : p3_4.x - p0.x);
	if (flex_height > _max_flex_height)
	    _max_flex_height = flex_height;
    } else {
	if (!_flex_message) {
	    _errh->lwarning(landmark(), "complex flex hint replaced with curves");
	    _errh->message("(This Type 2 format font contains flex hints prohibited by Type 1.\nI've safely replaced them with ordinary curves.)");
	    _flex_message = 1;
	}
	act_curve(cmd, p0, p1, p2, p3_4);
	act_curve(cmd, p3_4, p5, p6, p7);
    }
}

void
MakeType1CharstringInterp::act_closepath(int)
{
    gen_command(CS::cClosepath);
    _state = S_CLOSED;
}


// subroutines

MakeType1CharstringInterp::Subr *
MakeType1CharstringInterp::csr_subr(CsRef csr, bool force) const
{
    Vector<Subr *> *vp;
    if ((csr & CSR_TYPE) == CSR_SUBR)
	vp = &_subrs;
    else if ((csr & CSR_TYPE) == CSR_GSUBR)
	vp = &_gsubrs;
    else if ((csr & CSR_TYPE) == CSR_GLYPH)
	vp = &_glyphs;
    else
	return 0;
    
    int n = (csr & CSR_NUM);
    if (n >= vp->size())
	return 0;

    Subr *&what = (*vp)[n];
    if (!what && force)
	what = new Subr(csr);
    return what;
}

Type1Charstring *
MakeType1CharstringInterp::Subr::charstring(const MakeType1CharstringInterp *mcsi) const
{
    int n = (_csr & CSR_NUM);
    switch (_csr & CSR_TYPE) {
      case CSR_SUBR:
      case CSR_GSUBR:
	if (_output_subrno >= 0)
	    return static_cast<Type1Charstring *>(mcsi->output()->subr(_output_subrno));
	return 0;
      case CSR_GLYPH:
	return static_cast<Type1Charstring *>(mcsi->output()->glyph(n));
      default:
	return 0;
    }
}

void
MakeType1CharstringInterp::Subr::transfer_nested_calls(int pos, int length, Subr *new_caller) const
{
    int right = pos + length;
    for (int i = 0; i < _calls.size(); i++) {
	Subr *cs = _calls[i];
	for (int j = 0; j < cs->_callers.size(); j++) {
	    Caller &c = cs->_callers[j];
	    if (c.subr == this && pos <= c.pos && c.pos + c.len <= right) {
		c.subr = new_caller;
		c.pos -= pos;
		new_caller->add_call(cs);
	    }
	}
    }
}

void
MakeType1CharstringInterp::Subr::change_callers(Subr *caller, int pos, int length, int new_length)
{
    if (up_to_date())
	return;
    update();

    int right = pos + length;
    int delta = new_length - length;
    for (int i = 0; i < _callers.size(); i++) {
	Caller &c = _callers[i];
	if (c.subr != caller)
	    /* nada */;
	else if (pos <= c.pos && c.pos + c.len <= right) {
	    // erase
	    //fprintf(stderr, "  ERASE caller %08x:%d+%d\n", c.subr->_csr, c.pos, c.len);
	    c.subr = 0;
	} else if (right <= c.pos) {
	    //fprintf(stderr, "  ADJUST caller %08x:%d+%d -> %d+%d\n", c.subr->_csr, c.pos, c.len, c.pos+delta, c.len);
	    c.pos += delta;
	} else if (c.pos <= pos && right <= c.pos + c.len) {
	    c.len += delta;
	} else
	    c.subr = 0;
    }
}

void
MakeType1CharstringInterp::Subr::caller_data(bool assign, MakeType1CharstringInterp *mcsi)
{
    for (int i = 0; i < _callers.size(); i++) {
	Caller &c = _callers[i];
	if (!c.subr)
	    continue;
	String s = c.subr->charstring(mcsi)->substring(c.pos, c.len);
	if (assign)
	    c.s = s;
	else if (c.s != s) {
	    //fprintf(stderr, "FAIL %08x caller %08x:%d+%d\n", _csr, c.subr->_csr, c.pos, c.len);
	    //assert(0);
	}
	assert(c.subr->has_call(this));
    }
}

bool
MakeType1CharstringInterp::Subr::unify(MakeType1CharstringInterp *mcsi)
{
    // clean up caller list
    for (int i = 0; i < _callers.size(); i++)
	if (!_callers[i].subr) {
	    _callers[i] = _callers.back();
	    _callers.pop_back();
	    i--;
	}
    
    if (!_callers.size())
	return false;
    assert(!_calls.size());

    // Find the smallest shared substring.
    String substr = _callers[0].charstring(mcsi);
    int suboff = 0;
    for (int i = 1; i < _callers.size(); i++) {
	String substr1 = _callers[i].charstring(mcsi);
	const char *d = substr.data() + suboff, *d1 = substr1.data();
	const char *dx = substr.data() + substr.length(), *d1x = d1 + substr1.length();
	while (dx > d && d1x > d1 && dx[-1] == d1x[-1])
	    dx--, d1x--;
	suboff = dx - substr.data();
    }
    substr = substr.substring(Type1Charstring(substr).first_caret_after(suboff));
    if (!substr.length())
	return false;
    for (int i = 0; i < _callers.size(); i++) {
	Caller &c = _callers[i];
	if (int delta = c.len - substr.length()) {
	    c.pos += delta;
	    c.len -= delta;
	}
    }

    // otherwise, success
    _output_subrno = mcsi->output()->nsubrs();
    mcsi->output()->set_subr(_output_subrno, Type1Charstring(substr + "\013"));

    // note calls
    _callers[0].subr->transfer_nested_calls(_callers[0].pos, _callers[0].len, this);
    
    // adapt callers
    String callsubr_string = Type1CharstringGen::callsubr_string(_output_subrno);
    for (int i = 0; i < _callers.size(); i++)
	// 13.Jun.2003 - must check whether _callers[i].subr exists: if we
	// called a subroutine more than once, change_callers() might have
	// zeroed it out.
	if (_callers[i].subr && _callers[i].subr != this) {
	    Subr::Caller c = _callers[i];
	    //fprintf(stderr, " SUBSTRING %08x:%d+%d\n", c.subr->_csr, c.pos, c.len);
	    c.subr->charstring(mcsi)->assign_substring(c.pos, c.len, callsubr_string);
	    Subr::bump_date();
	    for (int j = 0; j < c.subr->ncalls(); j++)
		c.subr->call(j)->change_callers(c.subr, c.pos, c.len, callsubr_string.length());
	    assert(!_callers[i].subr);
	}

    // this subr is no longer "called"/interpolated from anywhere
    _callers.clear();

    //fprintf(stderr, "Succeeded %x\n", _csr);
    return true;
}


// running

bool
MakeType1CharstringInterp::type2_command(int cmd, const uint8_t *data, int *left)
{
    switch (cmd) {
    
      case CS::cCallsubr:
      case CS::cCallgsubr:
	if (subr_depth() < MAX_SUBR_DEPTH && size() == 1) {
	    //fprintf(stderr, "succeeded %d\n", (int) top());
	    bool g = (cmd == CS::cCallgsubr);
	    CsRef csref = ((int)top() + program()->xsubr_bias(g)) | (g ? CSR_GSUBR : CSR_SUBR);
	    Subr *callee = csr_subr(csref, true);
	    if (callee)
		_cur_subr->add_call(callee);

	    int left = _csgen.length();
	    
	    bool more = callxsubr_command(g);

	    int right = _csgen.length();
	    if (error() >= 0 && callee)
		callee->add_caller(_cur_subr, left, right - left);
	    return more;
	} else {
	    //fprintf(stderr, "failed %d\n", (int) top());
	    goto normal;
	}

      normal:
      default:
	return CharstringInterp::type2_command(cmd, data, left);
    
    }
}

void
MakeType1CharstringInterp::run(const CharstringContext &g, Type1Charstring &out, ErrorHandler *errh)
{
    _sidebearing = _width = Point(0, 0);
    _state = S_INITIAL;
    _csgen.clear();
    _stem_pos.clear();
    _stem_width.clear();
    _nhstem = 0;
    _errh = errh;
    
    CharstringInterp::interpret(g);
    
    if (_state == S_INITIAL)
	gen_sbw(false);
    if (_state != S_SEAC)
	_csgen.gen_command(CS::cEndchar);
    
    _csgen.output(out);
    _errh = 0;
}

void
MakeType1CharstringInterp::run(const CharstringProgram *program, Type1Font *output, PermString glyph_definer, ErrorHandler *errh)
{
    _output = output;
    _hr_firstsubr = output->nsubrs();

    _glyphs.assign(program->nglyphs(), 0);
    _subrs.assign(program->nsubrs(), 0);
    _subr_bias = program->subr_bias();
    _gsubrs.assign(program->ngsubrs(), 0);
    _gsubr_bias = program->gsubr_bias();

    // run over the glyphs
    int nglyphs = program->nglyphs();
    Type1Charstring receptacle;
    for (int i = 0; i < nglyphs; i++) {
	_cur_subr = _glyphs[i] = new Subr(CSR_GLYPH | i);
	_cur_glyph = i;
	run(program->glyph_context(i), receptacle, errh);
#if 0
	PermString n = program->glyph_name(i);
	if (1 || n == "one") {
	    fprintf(stderr, "%s was %s\n", n.c_str(), CharstringUnparser::unparse(*program->glyph(i)).c_str());
	    fprintf(stderr, "%s == %s\n", n.c_str(), CharstringUnparser::unparse(receptacle).c_str());
	}
#endif
	output->add_glyph(Type1Subr::make_glyph(program->glyph_name(i), receptacle, glyph_definer));
    }

    // unify Subrs
    for (int i = 0; i < _subrs.size(); i++)
	if (_subrs[i])
	    _subrs[i]->unify(this);

    for (int i = 0; i < _gsubrs.size(); i++)
	if (_gsubrs[i])
	    _gsubrs[i]->unify(this);
}


/*****
 * main
 **/

static void
add_number_def(Type1Font *output, int dict, PermString name, const Cff::Font *font, Cff::DictOperator op)
{
    double v;
    if (font->dict_value(op, 0, &v))
	output->add_definition(dict, Type1Definition::make(name, v, "def"));
}

static void
add_delta_def(Type1Font *output, int dict, PermString name, const Cff::Font *font, Cff::DictOperator op)
{
    Vector<double> vec;
    if (font->dict_value(op, vec)) {
	for (int i = 1; i < vec.size(); i++)
	    vec[i] += vec[i - 1];
	StringAccum sa;
	for (int i = 0; i < vec.size(); i++)
	    sa << (i ? ' ' : '[') << vec[i];
	sa << ']';
	output->add_definition(dict, Type1Definition::make_literal(name, sa.take_string(), (dict == Type1Font::dP ? "|-" : "readonly def")));
    }
}

Type1Font *
create_type1_font(Cff::Font *font, ErrorHandler *errh)
{
    String version = font->dict_string(Cff::oVersion);
    Type1Font *output = Type1Font::skeleton_make(font->font_name(), version);
    output->skeleton_comments_end();
    StringAccum sa;
    
    // FontInfo dictionary
    if (version)
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("version", version, "readonly def"));
    if (String s = font->dict_string(Cff::oNotice))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Notice", s, "readonly def"));
    if (String s = font->dict_string(Cff::oCopyright))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Copyright", s, "readonly def"));
    if (String s = font->dict_string(Cff::oFullName))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FullName", s, "readonly def"));
    if (String s = font->dict_string(Cff::oFamilyName))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FamilyName", s, "readonly def"));
    if (String s = font->dict_string(Cff::oWeight))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Weight", s, "readonly def"));
    double v;
    if (font->dict_value(Cff::oIsFixedPitch, 0, &v))
	output->add_definition(Type1Font::dFI, Type1Definition::make_literal("isFixedPitch", (v ? "true" : "false"), "def"));
    add_number_def(output, Type1Font::dFI, "ItalicAngle", font, Cff::oItalicAngle);
    add_number_def(output, Type1Font::dFI, "UnderlinePosition", font, Cff::oUnderlinePosition);
    add_number_def(output, Type1Font::dFI, "UnderlineThickness", font, Cff::oUnderlineThickness);
    output->skeleton_fontinfo_end();
    
    // Encoding, other font dictionary entries
    output->add_item(font->type1_encoding_copy());
    font->dict_value(Cff::oPaintType, 0, &v);
    output->add_definition(Type1Font::dF, Type1Definition::make("PaintType", v, "def"));
    output->add_definition(Type1Font::dF, Type1Definition::make("FontType", 1.0, "def"));
    Vector<double> vec;
    if (font->dict_value(Cff::oFontMatrix, vec) && vec.size() == 6) {
	sa << '[' << vec[0] << ' ' << vec[1] << ' ' << vec[2] << ' ' << vec[3] << ' ' << vec[4] << ' ' << vec[5] << ']';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontMatrix", sa.take_string(), "readonly def"));
    } else
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontMatrix", "[0.001 0 0 0.001 0 0]", "readonly def"));
    add_number_def(output, Type1Font::dF, "StrokeWidth", font, Cff::oStrokeWidth);
    add_number_def(output, Type1Font::dF, "UniqueID", font, Cff::oUniqueID);
    if (font->dict_value(Cff::oXUID, vec) && vec.size()) {
	for (int i = 0; i < vec.size(); i++)
	    sa << (i ? ' ' : '[') << vec[i];
	sa << ']';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("XUID", sa.take_string(), "readonly def"));
    }
    if (font->dict_value(Cff::oFontBBox, vec) && vec.size() == 4) {
	sa << '{' << vec[0] << ' ' << vec[1] << ' ' << vec[2] << ' ' << vec[3] << '}';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontBBox", sa.take_string(), "readonly def"));
    } else
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontBBox", "{0 0 0 0}", "readonly def"));
    output->skeleton_fontdict_end();

    // Private dictionary
    add_delta_def(output, Type1Font::dP, "BlueValues", font, Cff::oBlueValues);
    add_delta_def(output, Type1Font::dP, "OtherBlues", font, Cff::oOtherBlues);
    add_delta_def(output, Type1Font::dP, "FamilyBlues", font, Cff::oFamilyBlues);
    add_delta_def(output, Type1Font::dP, "FamilyOtherBlues", font, Cff::oFamilyOtherBlues);
    add_number_def(output, Type1Font::dP, "BlueScale", font, Cff::oBlueScale);
    add_number_def(output, Type1Font::dP, "BlueShift", font, Cff::oBlueShift);
    add_number_def(output, Type1Font::dP, "BlueFuzz", font, Cff::oBlueFuzz);
    if (font->dict_value(Cff::oStdHW, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("StdHW", String("[") + String(v) + "]", "|-"));
    if (font->dict_value(Cff::oStdVW, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("StdVW", String("[") + String(v) + "]", "|-"));
    add_delta_def(output, Type1Font::dP, "StemSnapH", font, Cff::oStemSnapH);
    add_delta_def(output, Type1Font::dP, "StemSnapV", font, Cff::oStemSnapV);
    if (font->dict_value(Cff::oForceBold, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("ForceBold", (v ? "true" : "false"), "def"));
    add_number_def(output, Type1Font::dP, "LanguageGroup", font, Cff::oLanguageGroup);
    add_number_def(output, Type1Font::dP, "ExpansionFactor", font, Cff::oExpansionFactor);
    add_number_def(output, Type1Font::dP, "UniqueID", font, Cff::oUniqueID);
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("MinFeature", "{16 16}", "|-"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("password", "5839", "def"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("lenIV", "0", "def"));
    output->skeleton_private_end();

    // add glyphs
    MakeType1CharstringInterp maker(5);
    maker.run(font, output, " |-", errh);
    
    return output;
}

#include <lcdf/vector.cc>
