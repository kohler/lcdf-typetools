#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "maket1font.hh"
#include <efont/t1interp.hh>
#include <efont/t1csgen.hh>
#include <lcdf/point.hh>
#include <efont/t1font.hh>
#include <efont/t1item.hh>
#include <efont/t1unparser.hh>

using namespace Efont;

static const char *othersubrs_code = "% Copyright (c) 1987-1990 Adobe Systems Incorporated.\n"
"% All Rights Reserved.\n"
"% This code to be used for Flex and hint replacement.\n"
"% Version 1.1\n"
"/OtherSubrs\n"
"[systemdict /internaldict known\n"
"{1183615869 systemdict /internaldict get exec\n"
"/FlxProc known {save true} {false} ifelse}\n"
"{userdict /internaldict known not {\n"
"userdict /internaldict\n"
"{count 0 eq\n"
"{/internaldict errordict /invalidaccess get exec} if\n"
"dup type /integertype ne\n"
"{/internaldict errordict /invalidaccess get exec} if\n"
"dup 1183615869 eq\n"
"{pop 0}\n"
"{/internaldict errordict /invalidaccess get exec}\n"
"ifelse\n"
"}\n"
"dup 14 get 1 25 dict put\n"
"bind executeonly put\n"
"} if\n"
"1183615869 userdict /internaldict get exec\n"
"/FlxProc known {save true} {false} ifelse}\n"
"ifelse\n"
"[\n"
"systemdict /internaldict known not\n"
"{ 100 dict /begin cvx /mtx matrix /def cvx } if\n"
"systemdict /currentpacking known {currentpacking true setpacking} if\n"
"{\n"
"systemdict /internaldict known {\n"
"1183615869 systemdict /internaldict get exec\n"
"dup /$FlxDict known not {\n"
"dup dup length exch maxlength eq\n"
"{ pop userdict dup /$FlxDict known not\n"
"{ 100 dict begin /mtx matrix def\n"
"\n"
"dup /$FlxDict currentdict put end } if }\n"
"{ 100 dict begin /mtx matrix def\n"
"dup /$FlxDict currentdict put end }\n"
"ifelse\n"
"} if\n"
"/$FlxDict get begin\n"
"} if\n"
"grestore\n"
"/exdef {exch def} def\n"
"/dmin exch abs 100 div def\n"
"/epX exdef /epY exdef\n"
"/c4y2 exdef /c4x2 exdef /c4y1 exdef /c4x1 exdef /c4y0 exdef /c4x0 exdef\n"
"/c3y2 exdef /c3x2 exdef /c3y1 exdef /c3x1 exdef /c3y0 exdef /c3x0 exdef\n"
"/c1y2 exdef /c1x2 exdef /c2x2 c4x2 def /c2y2 c4y2 def\n"
"/yflag c1y2 c3y2 sub abs c1x2 c3x2 sub abs gt def\n"
"/PickCoords {\n"
"{c1x0 c1y0 c1x1 c1y1 c1x2 c1y2 c2x0 c2y0 c2x1 c2y1 c2x2 c2y2 }\n"
"{c3x0 c3y0 c3x1 c3y1 c3x2 c3y2 c4x0 c4y0 c4x1 c4y1 c4x2 c4y2 }\n"
"ifelse\n"
"/y5 exdef /x5 exdef /y4 exdef /x4 exdef /y3 exdef /x3 exdef\n"
"/y2 exdef /x2 exdef /y1 exdef /x1 exdef /y0 exdef /x0 exdef\n"
"} def\n"
"mtx currentmatrix pop\n"
"mtx 0 get abs .00001 lt mtx 3 get abs .00001 lt or\n"
"{/flipXY -1 def }\n"
"{mtx 1 get abs .00001 lt mtx 2 get abs .00001 lt or\n"
"{/flipXY 1 def }\n"
"{/flipXY 0 def }\n"
"ifelse }\n"
"ifelse\n"
"/erosion 1 def\n"
"systemdict /internaldict known {\n"
"1183615869 systemdict /internaldict get exec dup\n"
"/erosion known\n"
"{/erosion get /erosion exch def}\n"
"{pop}\n"
"ifelse\n"
"} if\n"
"yflag\n"
"{flipXY 0 eq c3y2 c4y2 eq or\n"
"{false PickCoords }\n"
"{/shrink c3y2 c4y2 eq\n"
"{0}{c1y2 c4y2 sub c3y2 c4y2 sub div abs} ifelse def\n"
"/yshrink {c4y2 sub shrink mul c4y2 add} def\n"
"/c1y0 c3y0 yshrink def /c1y1 c3y1 yshrink def\n"
"/c2y0 c4y0 yshrink def /c2y1 c4y1 yshrink def\n"
"/c1x0 c3x0 def /c1x1 c3x1 def /c2x0 c4x0 def /c2x1 c4x1 def\n"
"/dY 0 c3y2 c1y2 sub round\n"
"dtransform flipXY 1 eq {exch} if pop abs def\n"
"dY dmin lt PickCoords\n"
"y2 c1y2 sub abs 0.001 gt {\n"
"c1x2 c1y2 transform flipXY 1 eq {exch} if\n"
"/cx exch def /cy exch def\n"
"/dY 0 y2 c1y2 sub round dtransform flipXY 1 eq {exch}\n"
"if pop def\n"
"dY round dup 0 ne\n"
"{/dY exdef }\n"
"{pop dY 0 lt {-1}{1} ifelse /dY exdef }\n"
"ifelse\n"
"/erode PaintType 2 ne erosion 0.5 ge and def\n"
"erode {/cy cy 0.5 sub def} if\n"
"/ey cy dY add def\n"
"/ey ey ceiling ey sub ey floor add def\n"
"erode {/ey ey 0.5 add def} if\n"
"ey cx flipXY 1 eq {exch} if itransform exch pop\n"
"y2 sub /eShift exch def\n"
"/y1 y1 eShift add def /y2 y2 eShift add def /y3 y3\n"
"eShift add def\n"
"} if\n"
"} ifelse\n"
"}\n"
"{flipXY 0 eq c3x2 c4x2 eq or\n"
"{false PickCoords }\n"
"{/shrink c3x2 c4x2 eq\n"
"{0}{c1x2 c4x2 sub c3x2 c4x2 sub div abs} ifelse def\n"
"/xshrink {c4x2 sub shrink mul c4x2 add} def\n"
"/c1x0 c3x0 xshrink def /c1x1 c3x1 xshrink def\n"
"/c2x0 c4x0 xshrink def /c2x1 c4x1 xshrink def\n"
"/c1y0 c3y0 def /c1y1 c3y1 def /c2y0 c4y0 def /c2y1 c4y1 def\n"
"/dX c3x2 c1x2 sub round 0 dtransform\n"
"flipXY -1 eq {exch} if pop abs def\n"
"dX dmin lt PickCoords\n"
"x2 c1x2 sub abs 0.001 gt {\n"
"c1x2 c1y2 transform flipXY -1 eq {exch} if\n"
"/cy exch def /cx exch def\n"
"/dX x2 c1x2 sub round 0 dtransform flipXY -1 eq {exch} if pop def\n"
"dX round dup 0 ne\n"
"{/dX exdef }\n"
"{pop dX 0 lt {-1}{1} ifelse /dX exdef }\n"
"ifelse\n"
"/erode PaintType 2 ne erosion .5 ge and def\n"
"erode {/cx cx .5 sub def} if\n"
"/ex cx dX add def\n"
"/ex ex ceiling ex sub ex floor add def\n"
"erode {/ex ex .5 add def} if\n"
"ex cy flipXY -1 eq {exch} if itransform pop\n"
"x2 sub /eShift exch def\n"
"/x1 x1 eShift add def /x2 x2 eShift add def /x3 x3 eShift add def\n"
"} if\n"
"} ifelse\n"
"} ifelse\n"
"x2 x5 eq y2 y5 eq or\n"
"{ x5 y5 lineto }\n"
"{ x0 y0 x1 y1 x2 y2 curveto\n"
"x3 y3 x4 y4 x5 y5 curveto }\n"
"ifelse\n"
"epY epX\n"
"}\n"
"systemdict /currentpacking known {exch setpacking} if\n"
"/exec cvx /end cvx ] cvx\n"
"executeonly\n"
"exch\n"
"{pop true exch restore}\n"
"{\n"
"systemdict /internaldict known not\n"
"{1183615869 userdict /internaldict get exec\n"
"exch /FlxProc exch put true}\n"
"{1183615869 systemdict /internaldict get exec\n"
"dup length exch maxlength eq\n"
"{false}\n"
"{1183615869 systemdict /internaldict get exec\n"
"exch /FlxProc exch put true}\n"
"ifelse}\n"
"ifelse}\n"
"ifelse\n"
"{systemdict /internaldict known\n"
"{{1183615869 systemdict /internaldict get exec /FlxProc get exec}}\n"
"{{1183615869 userdict /internaldict get exec /FlxProc get exec}}\n"
"ifelse executeonly\n"
"} if\n"
"{gsave currentpoint newpath moveto} executeonly\n"
"{currentpoint grestore gsave currentpoint newpath moveto}\n"
"executeonly\n"
"{systemdict /internaldict known not\n"
"{pop 3}\n"
"{1183615869 systemdict /internaldict get exec\n"
"dup /startlock known\n"
"{/startlock get exec}\n"
"{dup /strtlck known\n"
"{/strtlck get exec}\n"
"{pop 3}\n"
"ifelse}\n"
"ifelse}\n"
"ifelse\n"
"} executeonly\n"
"] noaccess def";

typedef unsigned CsRef;
enum { CSR_GLYPH = 0x00000000, CSR_SUBR = 0x80000000,
       CSR_GSUBR = 0xC0000000,
       CSR_TYPE = 0xC0000000, CSR_NUM = 0x3FFFFFFF };

class MakeType1CharstringInterp : public CharstringInterp { public:

    MakeType1CharstringInterp(EfontProgram *program, int precision = 5);
    ~MakeType1CharstringInterp();

    Type1Font *output() const			{ return _output; }
    
    void run(Type1Font *, PermString glyph_definer);
    void run(const Charstring &, Type1Charstring &);

    bool type2_command(int, const uint8_t *, int *);
    
    void char_sidebearing(int, double, double);
    void char_width(int, double, double);
    void char_seac(int, double, double, double, int, int);

    void char_hstem(int, double, double);
    void char_vstem(int, double, double);
    void char_hintmask(int, const unsigned char *, int);

    void char_rmoveto(int, double, double);
    void char_setcurrentpoint(int, double, double);
    void char_rlineto(int, double, double);
    void char_rrcurveto(int, double, double, double, double, double, double);
    void char_flex(int, double, double, double, double, double, double, double, double, double, double, double, double, double);
    void char_closepath(int);

    int nhints() const			{ return _stem_pos.size(); }
    double max_flex_height() const	{ return _max_flex_height; }

    class Subr;
    
  private:

    // output
    Type1CharstringGen _csgen;
    Type1Font *_output;

    // current glyph
    Point _sidebearing;
    Point _width;
    enum State { S_INITIAL, S_OPEN, S_CLOSED };
    State _state;

    Vector<double> _stem_pos;
    Vector<double> _stem_width;
    int _nhstem;

    // hint replacement
    Type1CharstringGen _hr_csgen;
    int _hr_firstsubr;

    // Flex
    double _max_flex_height;

    // subroutines
    int _subr_bias;
    int _gsubr_bias;
    mutable Vector<Subr *> _glyphs;
    mutable Vector<Subr *> _subrs;
    mutable Vector<Subr *> _gsubrs;

    Subr *_cur_subr;

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

    struct Callee {
	Subr *subr;
	int pos;
	int len;
	String s;
	Callee(Subr *s, int p, int l)	: subr(s), pos(p), len(l) { }
	String charstring(MakeType1CharstringInterp *mcsi) const { return subr->charstring(mcsi)->substring(pos, len); }
    };

    int ncallees() const		{ return _callees.size(); }
    const Callee &callee(int i) const	{ return _callees[i]; }
    
    void add_call(Subr *s)		{ _calls.push_back(s); }
    void add_callee(Subr *, int, int);

    int output_subrno() const		{ return _output_subrno; }
    void set_output_subrno(int n)	{ _output_subrno = n; }

    void transfer_nested_calls(int pos, int length, Subr *new_callee) const;
    void change_callees(Subr *, int pos, int length, int new_length);
    bool unify(MakeType1CharstringInterp *);
    
    void callee_data(bool assign, MakeType1CharstringInterp *mcsi);
    
  private:

    CsRef _csr;
    Vector<Subr *> _calls;
    Vector<Callee> _callees;

    int _output_subrno;
    int _stamp;

    static int max_stamp;

    friend class MakeType1CharstringInterp;
    
};

int MakeType1CharstringInterp::Subr::max_stamp = 1;

inline void
MakeType1CharstringInterp::Subr::add_callee(Subr *s, int pos, int len)
{
    _callees.push_back(Callee(s, pos, len));
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

MakeType1CharstringInterp::MakeType1CharstringInterp(EfontProgram *program, int precision)
    : CharstringInterp(program), _csgen(precision), _hr_csgen(precision),
      _hr_firstsubr(-1), _max_flex_height(0)
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
	char_hintmask(CS::cHintmask, reinterpret_cast<const unsigned char *>(s.data()), nhints());
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
MakeType1CharstringInterp::char_sidebearing(int, double x, double y)
{
    _sidebearing = Point(x, y);
}

void
MakeType1CharstringInterp::char_width(int, double x, double y)
{
    _width = Point(x, y);
}

void
MakeType1CharstringInterp::char_seac(int, double, double, double, int, int)
{
    assert(0);
}

void
MakeType1CharstringInterp::char_hstem(int, double pos, double width)
{
    if (_nhstem == _stem_pos.size()) {
	_stem_pos.push_back(pos);
	_stem_width.push_back(width);
	_nhstem++;
    }
}

void
MakeType1CharstringInterp::char_vstem(int, double pos, double width)
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
MakeType1CharstringInterp::char_hintmask(int cmd, const unsigned char *data, int nhints)
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
MakeType1CharstringInterp::char_rmoveto(int cmd, double dx, double dy)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    else if (_state == S_OPEN)
	char_closepath(cmd);
    if (dx == 0) {
	gen_number(dy, 'y');
	gen_command(CS::cVmoveto);
    } else if (dy == 0) {
	gen_number(dx, 'x');
	gen_command(CS::cHmoveto);
    } else {
	gen_number(dx, 'x');
	gen_number(dy, 'y');
	gen_command(CS::cRmoveto);
    }
}

void
MakeType1CharstringInterp::char_setcurrentpoint(int, double x, double y)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    gen_number(x, 'X');
    gen_number(y, 'Y');
    gen_command(CS::cSetcurrentpoint);
}

void
MakeType1CharstringInterp::char_rlineto(int, double dx, double dy)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    _state = S_OPEN;
    if (dx == 0) {
	gen_number(dy, 'y');
	gen_command(CS::cVlineto);
    } else if (dy == 0) {
	gen_number(dx, 'x');
	gen_command(CS::cHlineto);
    } else {
	gen_number(dx, 'x');
	gen_number(dy, 'y');
	gen_command(CS::cRlineto);
    }
}

void
MakeType1CharstringInterp::char_rrcurveto(int, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    _state = S_OPEN;
    if (dy1 == 0 && dx3 == 0) {
	gen_number(dx1, 'x');
	gen_number(dx2, 'x');
	gen_number(dy2, 'y');
	gen_number(dy3, 'y');
	gen_command(CS::cHvcurveto);
    } else if (dx1 == 0 && dy3 == 0) {
	gen_number(dy1, 'y');
	gen_number(dx2, 'x');
	gen_number(dy2, 'y');
	gen_number(dx3, 'x');
	gen_command(CS::cVhcurveto);
    } else {
	gen_number(dx1, 'x');
	gen_number(dy1, 'y');
	gen_number(dx2, 'x');
	gen_number(dy2, 'y');
	gen_number(dx3, 'x');
	gen_number(dy3, 'y');
	gen_command(CS::cRrcurveto);
    }
}

void
MakeType1CharstringInterp::char_flex(int cmd, double dx1, double dy1, double dx2, double dy2, double dx3, double dy3, double dx4, double dy4, double dx5, double dy5, double dx6, double dy6, double flex_depth)
{
    if (_state == S_INITIAL)
	gen_sbw(false);
    _state = S_OPEN;

    Point p0 = Point(_csgen.current_point_x(true), _csgen.current_point_y(true)),
	p1 = p0 + Point(dx1, dy1), p2 = p1 + Point(dx2, dy2),
	p3 = p2 + Point(dx3, dy3), p4 = p3 + Point(dx4, dy4),
	p5 = p4 + Point(dx5, dy5), p6 = p5 + Point(dx6, dy6);

    // 1. Outer endpoints must have same x (or y) coordinate
    bool v_ok = (p0.x == p6.x);
    bool h_ok = (p0.y == p6.y);
    
    // 2. Join point and its neighboring controls must be at an extreme
    if (v_ok && p2.x == p3.x && p3.x == p4.x) {
	double distance = fabs(p3.x - p0.x);
	int sign = (p3.x < p0.x ? -1 : 1);
	if (sign * (p1.x - p0.x) < 0 || sign * (p1.x - p0.x) > distance
	    || sign * (p5.x - p0.x) < 0 || sign * (p5.x - p0.x) > distance)
	    v_ok = false;
    } else
	v_ok = false;

    if (h_ok && p2.y == p3.y && p3.y == p4.y) {
	double distance = fabs(p3.y - p0.y);
	int sign = (p3.y < p0.y ? -1 : 1);
	if (sign * (p1.y - p0.y) < 0 || sign * (p1.y - p0.y) > distance
	    || sign * (p5.y - p0.y) < 0 || sign * (p5.y - p0.y) > distance)
	    h_ok = false;
    } else
	h_ok = false;

    // 3. Flex height <= 20
    if (v_ok && fabs(p3.x - p0.x) > 20)
	v_ok = false;
    if (h_ok && fabs(p3.y - p0.y) > 20)
	h_ok = false;

    // generate flex commands
    if (v_ok || h_ok) {
	Point p_reference = (h_ok ? Point(p3.x, p0.y) : Point(p0.x, p3.y));

	_csgen.gen_number(1);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, p_reference.x - p0.x, p_reference.y - p0.y);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, p1.x - p_reference.x, p1.y - p_reference.y);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, dx2, dy2);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, dx3, dy3);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, dx4, dy4);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, dx5, dy5);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);
	
	char_rmoveto(cmd, dx6, dy6);
	_csgen.gen_number(2);
	_csgen.gen_command(CS::cCallsubr);

	_csgen.gen_number(flex_depth);
	_csgen.gen_number(p6.x, 'X');
	_csgen.gen_number(p6.y, 'Y');
	_csgen.gen_number(0);
	_csgen.gen_command(CS::cCallsubr);

	double flex_height = fabs(h_ok ? p3.y - p0.y : p3.x - p0.x);
	if (flex_height > _max_flex_height)
	    _max_flex_height = flex_height;
	
	fprintf(stderr, "good flex\n");
    } else {
	fprintf(stderr, "bad flex: %g %g  %g %g  %g %g  %g %g  %g %g  %g %g\n", dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4, dx5, dy5, dx6, dy6);
	char_rrcurveto(cmd, dx1, dy1, dx2, dy2, dx3, dy3);
	char_rrcurveto(cmd, dx4, dy4, dx5, dy5, dx6, dy6);
    }
}

void
MakeType1CharstringInterp::char_closepath(int)
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
MakeType1CharstringInterp::Subr::transfer_nested_calls(int pos, int length, Subr *new_callee) const
{
    int right = pos + length;
    for (int i = 0; i < _calls.size(); i++) {
	Subr *cs = _calls[i];
	for (int j = 0; j < cs->_callees.size(); j++) {
	    Callee &c = cs->_callees[j];
	    if (c.subr == this && pos <= c.pos && c.pos + c.len <= right) {
		c.subr = new_callee;
		c.pos -= pos;
		new_callee->add_call(cs);
	    }
	}
    }
}

void
MakeType1CharstringInterp::Subr::change_callees(Subr *callee, int pos, int length, int new_length)
{
    if (up_to_date())
	return;
    update();

    int right = pos + length;
    int delta = new_length - length;
    for (int i = 0; i < _callees.size(); i++) {
	Callee &c = _callees[i];
	if (c.subr != callee)
	    /* nada */;
	else if (pos <= c.pos && c.pos + c.len <= right) {
	    // erase
	    //fprintf(stderr, "  ERASE callee %08x:%d+%d\n", c.subr->_csr, c.pos, c.len);
	    c.subr = 0;
	} else if (right <= c.pos) {
	    //fprintf(stderr, "  ADJUST callee %08x:%d+%d -> %d+%d\n", c.subr->_csr, c.pos, c.len, c.pos+delta, c.len);
	    c.pos += delta;
	} else if (c.pos <= pos && right <= c.pos + c.len) {
	    c.len += delta;
	} else
	    c.subr = 0;
    }
}

void
MakeType1CharstringInterp::Subr::callee_data(bool assign, MakeType1CharstringInterp *mcsi)
{
    for (int i = 0; i < _callees.size(); i++) {
	Callee &c = _callees[i];
	if (!c.subr)
	    continue;
	String s = c.subr->charstring(mcsi)->substring(c.pos, c.len);
	if (assign)
	    c.s = s;
	else if (c.s != s) {
	    //fprintf(stderr, "FAIL %08x callee %08x:%d+%d\n", _csr, c.subr->_csr, c.pos, c.len);
	    //assert(0);
	}
	assert(c.subr->has_call(this));
    }
}

bool
MakeType1CharstringInterp::Subr::unify(MakeType1CharstringInterp *mcsi)
{
    // clean up callee list
    for (int i = 0; i < _callees.size(); i++)
	if (!_callees[i].subr) {
	    _callees[i] = _callees.back();
	    _callees.pop_back();
	    i--;
	}
    
    if (!_callees.size())
	return false;
    assert(!_calls.size());

    // Find the smallest shared substring.
    String substr = _callees[0].charstring(mcsi);
    int suboff = 0;
    for (int i = 1; i < _callees.size(); i++) {
	String substr1 = _callees[i].charstring(mcsi);
	const char *d = substr.data() + suboff, *d1 = substr1.data();
	const char *dx = substr.data() + substr.length(), *d1x = d1 + substr1.length();
	while (dx > d && d1x > d1 && dx[-1] == d1x[-1])
	    dx--, d1x--;
	suboff = dx - substr.data();
    }
    substr = substr.substring(Type1Charstring(substr).first_caret_after(suboff));
    if (!substr.length())
	return false;
    for (int i = 0; i < _callees.size(); i++) {
	Callee &c = _callees[i];
	if (int delta = c.len - substr.length()) {
	    c.pos += delta;
	    c.len -= delta;
	}
    }

    // otherwise, success
    _output_subrno = mcsi->output()->nsubrs();
    mcsi->output()->set_subr(_output_subrno, Type1Charstring(substr + "\013"));

    // note calls
    _callees[0].subr->transfer_nested_calls(_callees[0].pos, _callees[0].len, this);
    
    // adapt callers
    String callsubr_string = Type1CharstringGen::callsubr_string(_output_subrno);
    for (int i = 0; i < _callees.size(); i++)
	if (_callees[i].subr != this) {
	    Subr::Callee c = _callees[i];
	    //fprintf(stderr, " SUBSTRING %08x:%d+%d\n", c.subr->_csr, c.pos, c.len);
	    c.subr->charstring(mcsi)->assign_substring(c.pos, c.len, callsubr_string);
	    Subr::bump_date();
	    for (int j = 0; j < c.subr->ncalls(); j++)
		c.subr->call(j)->change_callees(c.subr, c.pos, c.len, callsubr_string.length());
	    assert(!_callees[i].subr);
	}

    // this subr is no longer "called"/interpolated from anywhere
    _callees.clear();

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
		callee->add_callee(_cur_subr, left, right - left);
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
MakeType1CharstringInterp::run(const Charstring &cs, Type1Charstring &out)
{
    _sidebearing = _width = Point(0, 0);
    _state = S_INITIAL;
    _csgen.clear();
    _stem_pos.clear();
    _stem_width.clear();
    _nhstem = 0;
    CharstringInterp::init();
    
    cs.run(*this);
    if (_state == S_INITIAL)
	gen_sbw(false);
    _csgen.gen_command(CS::cEndchar);
    
    _csgen.output(out);
}

void
MakeType1CharstringInterp::run(Type1Font *output, PermString glyph_definer)
{
    _output = output;
    _hr_firstsubr = output->nsubrs();

    const EfontProgram *p = program();
    _glyphs.assign(p->nglyphs(), 0);
    _subrs.assign(p->nsubrs(), 0);
    _subr_bias = p->subr_bias();
    _gsubrs.assign(p->ngsubrs(), 0);
    _gsubr_bias = p->gsubr_bias();

    // run over the glyphs
    int nglyphs = p->nglyphs();
    Type1Charstring receptacle;
    for (int i = 0; i < nglyphs; i++) {
	_cur_subr = _glyphs[i] = new Subr(CSR_GLYPH | i);
	run(*p->glyph(i), receptacle);
	output->add_glyph(Type1Subr::make_glyph(p->glyph_name(i), receptacle, glyph_definer));
    }

    // unify Subrs
    for (int i = 0; i < _subrs.size(); i++)
	if (_subrs[i] && _subrs[i]->unify(this))
	    ;

    for (int i = 0; i < _gsubrs.size(); i++)
	if (_gsubrs[i] && _gsubrs[i]->unify(this))
	    ;
}


/*****
 * main
 **/

static void
add_number_def(Type1Font *output, int dict, PermString name, const EfontCFF::Font *font, EfontCFF::DictOperator op)
{
    double v;
    if (font->dict_value(op, 0, &v))
	output->add_definition(dict, Type1Definition::make(name, v, "def"));
}

static void
add_delta_def(Type1Font *output, int dict, PermString name, const EfontCFF::Font *font, EfontCFF::DictOperator op)
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
create_type1_font(EfontCFF::Font *font)
{
    Type1Font *output = new Type1Font(font->font_name());

    // %!PS-Adobe-Font comment
    StringAccum sa;
    sa << "%!PS-AdobeFont-1.0: " << font->font_name();
    String version = font->dict_string(EfontCFF::oVersion);
    if (version)
	sa << ' ' << version;
    output->add_item(new Type1CopyItem(sa.take_string()));

    // count members of font dictionary
    int nfont_dict = 4		// FontName, Private, FontInfo, Encoding
	+ 4			// PaintType, FontType, FontMatrix, FontBBox
	+ font->dict_has(EfontCFF::oUniqueID)
	+ font->dict_has(EfontCFF::oXUID)
	+ 2;			// padding
    sa << nfont_dict << " dict begin";
    output->add_item(new Type1CopyItem(sa.take_string()));
    output->add_definition(Type1Font::dF, new Type1Definition("FontName", "/" + String(font->font_name()), "def"));
		     
    // FontInfo dictionary
    int nfont_info_dict = ((bool)version)
	+ font->dict_has(EfontCFF::oNotice)
	+ font->dict_has(EfontCFF::oCopyright)
	+ font->dict_has(EfontCFF::oFullName)
	+ font->dict_has(EfontCFF::oFamilyName)
	+ font->dict_has(EfontCFF::oWeight)
	+ font->dict_has(EfontCFF::oIsFixedPitch)
	+ font->dict_has(EfontCFF::oItalicAngle)
	+ font->dict_has(EfontCFF::oUnderlinePosition)
	+ font->dict_has(EfontCFF::oUnderlineThickness)
	+ 2;			// padding
    sa << "/FontInfo " << nfont_info_dict << " dict dup begin";
    output->add_item(new Type1CopyItem(sa.take_string()));
    if (version)
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("version", version, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oNotice))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Notice", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oCopyright))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Copyright", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oFullName))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FullName", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oFamilyName))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("FamilyName", s, "readonly def"));
    if (String s = font->dict_string(EfontCFF::oWeight))
	output->add_definition(Type1Font::dFI, Type1Definition::make_string("Weight", s, "readonly def"));
    double v;
    if (font->dict_value(EfontCFF::oIsFixedPitch, 0, &v))
	output->add_definition(Type1Font::dFI, Type1Definition::make_literal("isFixedPitch", (v ? "true" : "false"), "def"));
    add_number_def(output, Type1Font::dFI, "ItalicAngle", font, EfontCFF::oItalicAngle);
    add_number_def(output, Type1Font::dFI, "UnderlinePosition", font, EfontCFF::oUnderlinePosition);
    add_number_def(output, Type1Font::dFI, "UnderlineThickness", font, EfontCFF::oUnderlineThickness);
    output->add_item(new Type1CopyItem("end readonly def"));
    
    // Encoding
    output->add_item(font->type1_encoding_copy());

    // other font dictionary entries
    font->dict_value(EfontCFF::oPaintType, 0, &v);
    output->add_definition(Type1Font::dF, Type1Definition::make("PaintType", v, "def"));
    output->add_definition(Type1Font::dF, Type1Definition::make("FontType", 1.0, "def"));
    Vector<double> vec;
    if (font->dict_value(EfontCFF::oFontMatrix, vec) && vec.size() == 6) {
	sa << '[' << vec[0] << ' ' << vec[1] << ' ' << vec[2] << ' ' << vec[3] << ' ' << vec[4] << ' ' << vec[5] << ']';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontMatrix", sa.take_string(), "readonly def"));
    } else
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontMatrix", "[0.001 0 0 0.001 0 0]", "readonly def"));
    add_number_def(output, Type1Font::dF, "StrokeWidth", font, EfontCFF::oStrokeWidth);
    add_number_def(output, Type1Font::dF, "UniqueID", font, EfontCFF::oUniqueID);
    if (font->dict_value(EfontCFF::oXUID, vec) && vec.size()) {
	for (int i = 0; i < vec.size(); i++)
	    sa << (i ? ' ' : '[') << vec[i];
	sa << ']';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("XUID", sa.take_string(), "readonly def"));
    }
    if (font->dict_value(EfontCFF::oFontBBox, vec) && vec.size() == 4) {
	sa << '{' << vec[0] << ' ' << vec[1] << ' ' << vec[2] << ' ' << vec[3] << '}';
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontBBox", sa.take_string(), "readonly def"));
    } else
	output->add_definition(Type1Font::dF, Type1Definition::make_literal("FontBBox", "{0 0 0 0}", "readonly def"));

    // switch to eexec
    output->add_item(new Type1CopyItem("currentdict end"));
    output->add_item(new Type1CopyItem("currentfile eexec"));
    output->add_item(new Type1EexecItem(true));

    // Private dictionary
    int nprivate_dict = 4	// CharStrings, Subrs, lenIV, password
	+ 5			// MinFeature, |-, -|, |, OtherSubrs
	+ font->dict_has(EfontCFF::oUniqueID)
	+ font->dict_has(EfontCFF::oBlueValues)
	+ font->dict_has(EfontCFF::oOtherBlues)
	+ font->dict_has(EfontCFF::oFamilyBlues)
	+ font->dict_has(EfontCFF::oFamilyOtherBlues)
	+ font->dict_has(EfontCFF::oBlueScale)
	+ font->dict_has(EfontCFF::oBlueShift)
	+ font->dict_has(EfontCFF::oBlueFuzz)
	+ font->dict_has(EfontCFF::oStdHW)
	+ font->dict_has(EfontCFF::oStdVW)
	+ font->dict_has(EfontCFF::oStemSnapH)
	+ font->dict_has(EfontCFF::oStemSnapV)
	+ font->dict_has(EfontCFF::oForceBold)
	+ font->dict_has(EfontCFF::oLanguageGroup)
	+ font->dict_has(EfontCFF::oExpansionFactor)
	+ 2;			// padding
    sa << "dup /Private " << nprivate_dict << " dict dup begin";
    output->add_item(new Type1CopyItem(sa.take_string()));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("-|", "{string currentfile exch readstring pop}", "executeonly def"));
    output->set_charstring_definer(" -| ");
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("|-", "{noaccess def}", "executeonly def"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("|", "{noaccess put}", "executeonly def"));
    add_delta_def(output, Type1Font::dP, "BlueValues", font, EfontCFF::oBlueValues);
    add_delta_def(output, Type1Font::dP, "OtherBlues", font, EfontCFF::oOtherBlues);
    add_delta_def(output, Type1Font::dP, "FamilyBlues", font, EfontCFF::oFamilyBlues);
    add_delta_def(output, Type1Font::dP, "FamilyOtherBlues", font, EfontCFF::oFamilyOtherBlues);
    add_number_def(output, Type1Font::dP, "BlueScale", font, EfontCFF::oBlueScale);
    add_number_def(output, Type1Font::dP, "BlueShift", font, EfontCFF::oBlueShift);
    add_number_def(output, Type1Font::dP, "BlueFuzz", font, EfontCFF::oBlueFuzz);
    if (font->dict_value(EfontCFF::oStdHW, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("StdHW", String("[") + String(v) + "]", "|-"));
    if (font->dict_value(EfontCFF::oStdVW, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("StdVW", String("[") + String(v) + "]", "|-"));
    add_delta_def(output, Type1Font::dP, "StemSnapH", font, EfontCFF::oStemSnapH);
    add_delta_def(output, Type1Font::dP, "StemSnapV", font, EfontCFF::oStemSnapV);
    if (font->dict_value(EfontCFF::oForceBold, 0, &v))
	output->add_definition(Type1Font::dP, Type1Definition::make_literal("ForceBold", (v ? "true" : "false"), "def"));
    add_number_def(output, Type1Font::dP, "LanguageGroup", font, EfontCFF::oLanguageGroup);
    add_number_def(output, Type1Font::dP, "ExpansionFactor", font, EfontCFF::oExpansionFactor);
    add_number_def(output, Type1Font::dP, "UniqueID", font, EfontCFF::oUniqueID);
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("MinFeature", "{16 16}", "|-"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("password", "5839", "def"));
    output->add_definition(Type1Font::dP, Type1Definition::make_literal("lenIV", "0", "def"));
    output->add_item(new Type1CopyItem(othersubrs_code));

    // Subrs
    sa << "/Subrs 4 array";
    output->add_item(new Type1SubrGroupItem(output, true, sa.take_string()));
    output->add_item(new Type1CopyItem("|-"));

    // - first four Subrs have fixed definitions
    // - 0: "3 0 callothersubr pop pop setcurrentpoint return"
    output->set_subr(0, Type1Charstring(String::stable_string("\216\213\014\020\014\021\014\021\014\041\013", 11)), " |");
    // - 1: "0 1 callothersubr return"
    output->set_subr(1, Type1Charstring(String::stable_string("\213\214\020\014\013", 5)), " |");
    // - 2: "0 2 callothersubr return"
    output->set_subr(2, Type1Charstring(String::stable_string("\213\215\020\014\013", 5)), " |");
    // - 3: "return"
    output->set_subr(3, Type1Charstring(String::stable_string("\013", 1)), " |");
    // - 4: "1 3 callothersubr pop callsubr return"
    output->set_subr(4, Type1Charstring(String::stable_string("\214\216\014\020\014\021\012\013", 8)), " |");
    
    // CharStrings
    sa << "2 index /CharStrings " << font->nglyphs() << " dict dup begin";
    output->add_item(new Type1SubrGroupItem(output, false, sa.take_string()));

    // completion
    output->add_item(new Type1CopyItem("end\n\
end\n\
readonly put\n\
noaccess put\n\
dup /FontName get exch definefont pop\n\
mark currentfile closefile"));
    output->add_item(new Type1EexecItem(false));
    output->add_item(new Type1CopyItem("\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
0000000000000000000000000000000000000000000000000000000000000000\n\
cleartomark"));

    // add glyphs
    MakeType1CharstringInterp maker(font, 5);
    maker.run(output, " |-");
    
    return output;
}

#include <lcdf/vector.cc>
