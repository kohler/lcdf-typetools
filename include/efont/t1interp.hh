// -*- related-file-name: "../../libefont/t1interp.cc" -*-
#ifndef EFONT_T1INTERP_HH
#define EFONT_T1INTERP_HH
#include <efont/t1cs.hh>
#include <cstdio>
namespace Efont {

// Allow unknown doubles to have some `fuzz' -- so if an unknown double
// is a bit off from the canonical UNKDOUBLE value, we'll still recognize
// it as unknown. (Useful for interpolation.)
#define UNKDOUBLE		-9.79797e97
#define MIN_KNOWN_DOUBLE	-9.69696e97
#define KNOWN(d)		((d) >= MIN_KNOWN_DOUBLE)

class CharstringInterp { public:

    CharstringInterp(const EfontProgram *, Vector<double> *weight = 0);
    virtual ~CharstringInterp()			{ }

    int error() const				{ return _error; }
    int error_data() const			{ return _error_data; }
    static String error_string(int error, int error_data);
    String error_string() const;
    
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

    int subr_depth() const			{ return _subr_depth; }
    
    double &vec(Vector<double> *, int);
    Vector<double> *weight_vector();
    Vector<double> *scratch_vector()		{ return &_scratch_vector; }

    const EfontProgram *program() const		{ return _program; }
    Charstring *get_subr(int) const;
    Charstring *get_gsubr(int) const;
    Charstring *get_xsubr(bool g, int) const;
    Charstring *get_glyph(PermString) const;

    virtual void init();
    bool error(int c)				{ return error(c, 0); }
    virtual bool error(int, int);
    virtual bool number(double);

    bool arith_command(int);
    bool vector_command(int);
    bool blend_command();
    bool callsubr_command();
    bool callgsubr_command();
    bool callxsubr_command(bool g);
    bool mm_command(int, int);
    bool itc_command(int, int);
    
    virtual bool callothersubr_command(int, int);
    virtual bool type1_command(int);
    virtual bool type2_command(int, const uint8_t *, int *);

    virtual void char_sidebearing(int, double, double);
    virtual void char_width(int, double, double);
    virtual void char_default_width(int);
    virtual void char_nominal_width_delta(int, double);
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
    virtual void char_hintmask(int, const uint8_t *, int);

    typedef Charstring CS;
    
    enum Errors {
	errOK		= 0,
	errInternal	= -1,
	errRunoff	= -2,
	errUnimplemented = -3,
	errOverflow	= -4,
	errUnderflow	= -5,
	errVector	= -6,
	errValue	= -7,
	errSubr		= -8,
	errGlyph	= -9,
	errCurrentPoint	= -10,
	errFlex		= -11,
	errMultipleMaster = -12,
	errOpenStroke	= -13,
	errLateSidebearing = -14,
	errOthersubr	= -15,
	errOrdering	= -16,
	errHintmask	= -17,
	errSubrDepth	= -18,
	errLastError	= -18
    };

    enum { STACK_SIZE = 48, PS_STACK_SIZE = 24, MAX_SUBR_DEPTH = 10,
	   SCRATCH_SIZE = 32 };
    
  private:
  
    int _error;
    int _error_data;
    bool _done;

    double _s[STACK_SIZE];
    int _sp;
    double _ps_s[PS_STACK_SIZE];
    int _ps_sp;

    int _subr_depth;

    Vector<double> *_weight_vector;
    Vector<double> _scratch_vector;

    double _lsbx;
    double _lsby;
  
    const EfontProgram *_program;

    // for processing Type 2 charstrings
    enum Type2State {
	T2_INITIAL, T2_HSTEM, T2_VSTEM, T2_HINTMASK, T2_PATH
    };
    Type2State _t2state;
    int _t2nhints;
    
    static double double_for_error;
  
    bool roll_command();
    int type2_handle_width(int, bool);

};


inline String
CharstringInterp::error_string() const
{
    return error_string(_error, _error_data);
}

inline void
CharstringInterp::push(double d)
{
    if (_sp < STACK_SIZE)
	_s[_sp++] = d;
    else
	error(errOverflow);
}

inline void
CharstringInterp::ps_push(double d)
{
    if (_ps_sp < PS_STACK_SIZE)
	_ps_s[_ps_sp++] = d;
    else
	error(errOverflow);
}

inline double &
CharstringInterp::vec(Vector<double> *v, int i)
{
    if (i < 0 || i >= v->size()) {
	error(errVector);
	return double_for_error;
    } else
	return v->at_u(i);
}

inline Charstring *
CharstringInterp::get_subr(int n) const
{
    return _program ? _program->subr(n) : 0;
}

inline Charstring *
CharstringInterp::get_gsubr(int n) const
{
    return _program ? _program->gsubr(n) : 0;
}

inline Charstring *
CharstringInterp::get_xsubr(bool g, int n) const
{
    return _program ? _program->xsubr(g, n) : 0;
}

inline Charstring *
CharstringInterp::get_glyph(PermString n) const
{
    return _program ? _program->glyph(n) : 0;
}

inline Vector<double> *
CharstringInterp::weight_vector()
{
    if (!_weight_vector && _program)
	_weight_vector = _program->weight_vector();
    return _weight_vector;
}

inline bool
CharstringInterp::callxsubr_command(bool g)
{
    return (g ? callgsubr_command() : callsubr_command());
}

}
#endif
