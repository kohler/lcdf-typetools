#ifndef T1INTERP_HH
#define T1INTERP_HH
#include "vector.hh"
#include "t1cs.hh"

class Type1Interp { public:

    Type1Interp(const PsfontProgram *, Vector<double> *weight = 0);
    virtual ~Type1Interp()			{ }

    int errno() const				{ return _errno; }
    int error_data() const			{ return _error_data; }
    bool done() const				{ return _done; }
    void set_done()				{ _done = true; }

    int size() const				{ return _sp; }
    double &at(unsigned i)			{ return _s[i]; }
    double &top(unsigned i = 0)			{ return _s[_sp - i - 1]; }
    double pop(unsigned n = 1)			{ _sp -= n; return _s[_sp]; }
    void push(double);
    void clear()				{ _sp = 0; }

    int ps_size() const				{ return _ps_sp; }
    double ps_at(unsigned i) const		{ return _ps_s[i]; }
    double ps_pop()				{ return _ps_s[--_ps_sp]; }
    void ps_push(double);
    void ps_clear()				{ _ps_sp = 0; }

    double &vec(Vector<double> *, int);
    Vector<double> *weight_vector();
    Vector<double> *scratch_vector()		{ return &_scratch_vector; }
  
    PsfontCharstring *get_subr(int) const;
    PsfontCharstring *get_gsubr(int) const;
    PsfontCharstring *get_glyph(PermString) const;

    virtual void init();
    bool error(int c)				{ return error(c, 0); }
    virtual bool error(int, int);
    virtual bool number(double);

    bool arith_command(int);
    bool vector_command(int);
    bool blend_command();
    bool callsubr_command();
    bool callgsubr_command();
    bool mm_command(int, int);
    bool itc_command(int, int);
    
    virtual bool callothersubr_command(int, int);
    virtual bool type1_command(int);
    virtual bool type2_command(int, const unsigned char *, int *);

    virtual void char_sidebearing(int, double, double);
    virtual void char_width(int, double, double);
    virtual void char_width_delta(int, double);
    virtual void char_seac(int, double, double, double, int, int);

    virtual void char_rmoveto(int, double, double);
    virtual void char_setcurrentpoint(int, double, double);
    virtual void char_rlineto(int, double, double);
    virtual void char_rrcurveto(int, double, double, double, double, double, double);
    virtual void char_flex(int, double, double, double, double, double, double, double, double, double, double, double, double, double);
    virtual void char_closepath(int);

    virtual void char_hstem(int, double, double);
    virtual void char_vstem(int, double, double);
    virtual void char_hstem3(int, double, double, double, double, double, double);
    virtual void char_vstem3(int, double, double, double, double, double, double);

    enum Commands {
	cError		= 0,
	cHstem		= 1,
	cVstem		= 3,
	cVmoveto	= 4,
	cRlineto	= 5,
	cHlineto	= 6,
	cVlineto	= 7,
	cRrcurveto	= 8,
	cClosepath	= 9,
	cCallsubr	= 10,
	cReturn		= 11,
	cEscape		= 12,
	cHsbw		= 13,
	cEndchar	= 14,
	cBlend		= 16,
	cHstemhm	= 18,
	cHintmask	= 19,
	cCntrmask	= 20,
	cRmoveto	= 21,
	cHmoveto	= 22,
	cVstemhm	= 23,
	cRcurveline	= 24,
	cRlinecurve	= 25,
	cVvcurveto	= 26,
	cHhcurveto	= 27,
	cShortint	= 28,
	cCallgsubr	= 29,
	cVhcurveto	= 30,
	cHvcurveto	= 31,
    
	cEscapeDelta	= 32,
	cDotsection	= 32 + 0,
	cVstem3		= 32 + 1,
	cHstem3		= 32 + 2,
	cAnd		= 32 + 3,
	cOr		= 32 + 4,
	cNot		= 32 + 5,
	cSeac		= 32 + 6,
	cSbw		= 32 + 7,
	cStore		= 32 + 8,
	cAbs		= 32 + 9,
	cAdd		= 32 + 10,
	cSub		= 32 + 11,
	cDiv		= 32 + 12,
	cLoad		= 32 + 13,
	cNeg		= 32 + 14,
	cEq		= 32 + 15,
	cCallothersubr	= 32 + 16,
	cPop		= 32 + 17,
	cDrop		= 32 + 18,
	cPut		= 32 + 20,
	cGet		= 32 + 21,
	cIfelse		= 32 + 22,
	cRandom		= 32 + 23,
	cMul		= 32 + 24,
	cSqrt		= 32 + 26,
	cDup		= 32 + 27,
	cExch		= 32 + 28,
	cIndex		= 32 + 29,
	cRoll		= 32 + 30,
	cSetcurrentpoint = 32 + 33,
	cHflex		= 32 + 34,
	cFlex		= 32 + 35,
	cHflex1		= 32 + 36,
	cFlex1		= 32 + 37,

	cLastCommand	= cFlex1
    };

    enum OthersubrCommands {
	othcFlexend = 0,
	othcFlexbegin = 1,
	othcFlexmiddle = 2,
	othcReplacehints = 3,
	othcMM1 = 14,
	othcMM2 = 15,
	othcMM3 = 16,
	othcMM4 = 17,
	othcMM6 = 18,
	othcITC_load = 19,
	othcITC_add = 20,
	othcITC_sub = 21,
	othcITC_mul = 22,
	othcITC_div = 23,
	othcITC_put = 24,
	othcITC_get = 25,
	othcITC_unknown = 26,
	othcITC_ifelse = 27,
	othcITC_random = 28,
    };
  
    enum Errors {
	errOK		= 0,
	errInternal	= 1,
	errRunoff	= 2,
	errUnimplemented = 3,
	errOverflow	= 4,
	errUnderflow	= 5,
	errVector	= 6,
	errValue	= 7,
	errSubr		= 8,
	errGlyph	= 9,
	errCurrentPoint	= 10,
	errFlex		= 11,
	errMultipleMaster = 12,
	errOpenStroke	= 13,
	errLateSidebearing = 14,
	errOthersubr	= 15,
	errOrdering	= 16,
	errHintmask	= 17,
	errLastError	= 17
    };
  
    enum { StackSize = 24, ScratchSize = 32 };
  
  private:
  
    int _errno;
    int _error_data;
    bool _done;

    double _s[StackSize];
    int _sp;
    double _ps_s[StackSize];
    int _ps_sp;
  
    Vector<double> *_weight_vector;
    Vector<double> _scratch_vector;

    double _lsbx;
    double _lsby;
  
    const PsfontProgram *_program;

    // for processing Type 2 charstrings
    enum Type2State {
	T2_INITIAL, T2_HSTEM, T2_VSTEM, T2_HINTMASK, T2_PATH
    };
    Type2State _t2state;
    int _t2nhints;
    
    static double double_for_error;
  
    bool roll_command();

};


inline void
Type1Interp::push(double d)
{
    if (_sp < StackSize)
	_s[_sp++] = d;
    else
	_errno = errOverflow;
}

inline void
Type1Interp::ps_push(double d)
{
    if (_ps_sp < StackSize)
	_ps_s[_ps_sp++] = d;
    else
	_errno = errOverflow;
}

inline double &
Type1Interp::vec(Vector<double> *v, int i)
{
    if (i < 0 || i >= v->size()) {
	_errno = errVector;
	return double_for_error;
    } else
	return v->at_u(i);
}

inline PsfontCharstring *
Type1Interp::get_subr(int n) const
{
    return _program ? _program->subr(n) : 0;
}

inline PsfontCharstring *
Type1Interp::get_gsubr(int n) const
{
    return _program ? _program->gsubr(n) : 0;
}

inline PsfontCharstring *
Type1Interp::get_glyph(PermString n) const
{
    return _program ? _program->glyph(n) : 0;
}

inline Vector<double> *
Type1Interp::weight_vector()
{
    if (!_weight_vector && _program)
	_weight_vector = _program->weight_vector();
    return _weight_vector;
}

#endif
