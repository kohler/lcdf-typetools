#ifndef T1INTERP_HH
#define T1INTERP_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "vector.hh"
#include "t1cs.hh"

class Type1Interp {
 public:
  
  enum Commands {
    
    cError	= 0,
    cHstem	= 1,
    cVstem	= 3,
    cVmoveto	= 4,
    cRlineto	= 5,
    cHlineto	= 6,
    cVlineto	= 7,
    cRrcurveto	= 8,
    cClosepath	= 9,
    cCallsubr	= 10,
    cReturn	= 11,
    cEscape	= 12,
    cHsbw	= 13,
    cEndchar	= 14,
    cBlend	= 16,
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
    
    cEscapeDelta= 32,
    cDotsection	= 32 + 0,
    cVstem3	= 32 + 1,
    cHstem3	= 32 + 2,
    cAnd	= 32 + 3,
    cOr		= 32 + 4,
    cNot	= 32 + 5,
    cSeac	= 32 + 6,
    cSbw	= 32 + 7,
    cStore	= 32 + 8,
    cAbs	= 32 + 9,
    cAdd	= 32 + 10,
    cSub	= 32 + 11,
    cDiv	= 32 + 12,
    cLoad	= 32 + 13,
    cNeg	= 32 + 14,
    cEq		= 32 + 15,
    cCallothersubr = 32 + 16,
    cPop	= 32 + 17,
    cDrop	= 32 + 18,
    cPut	= 32 + 20,
    cGet	= 32 + 21,
    cIfelse	= 32 + 22,
    cRandom	= 32 + 23,
    cMul	= 32 + 24,
    cSqrt	= 32 + 26,
    cDup	= 32 + 27,
    cExch	= 32 + 28,
    cIndex	= 32 + 29,
    cRoll	= 32 + 30,
    cSetcurrentpoint = 32 + 33,
    
    cLastCommand = cSetcurrentpoint
    
  };
  
  enum Errors {
    errOK		= 0,
    errInternal		= 1,
    errRunoff		= 2,
    errUnimplemented	= 3,
    errOverflow		= 4,
    errUnderflow	= 5,
    errVector		= 6,
    errValue		= 7,
    errSubr		= 8,
    errGlyph		= 9,
    errCurrentPoint	= 10,
    errFlex		= 11,
    errMultipleMaster	= 12,
  };
  
 private:
  
  const int StackSize	= 24;
  const int ScratchSize	= 32;
  
  int _error;
  bool _done;
  
  double _s[StackSize];
  int _sp;
  double _ps_s[StackSize];
  int _ps_sp;
  
  Vector<double> *_weight_vector;
  Vector<double> _scratch_vector;
  
  const Type1Program *_program;
  
  static double double_for_error;
  
  bool blend_command();
  bool roll_command();
  
 public:
  
  Type1Interp(const Type1Program *, Vector<double> *weight = 0);
  virtual ~Type1Interp()			{ }
  
  int error() const				{ return _error; }
  bool done() const				{ return _done; }
  void set_done()				{ _done = true; }
  
  int count() const				{ return _sp; }
  double &at(unsigned i)			{ return _s[i]; }
  double &top(unsigned i = 0)			{ return _s[ _sp - i - 1 ]; }
  double pop(unsigned n = 1)			{ _sp -= n; return _s[_sp]; }
  void push(double);
  void clear()					{ _sp = 0; }
  
  int count_ps() const				{ return _ps_sp; }
  double at_ps(unsigned i) const		{ return _ps_s[i]; }
  double pop_ps()				{ return _ps_s[--_ps_sp]; }
  void push_ps(double);
  void clear_ps()				{ _ps_sp = 0; }
  
  double &vec(Vector<double> *, int);
  Vector<double> *weight_vector();
  
  Type1Charstring *get_subr(int) const;
  Type1Charstring *get_glyph(PermString) const;
  
  virtual void init();
  virtual bool error(int);
  virtual bool number(double);
  
  bool arith_command(int);
  bool vector_command(int);
  bool callsubr_command();
  bool mm_command(int, int);
  virtual bool command(int);
  
};


inline void
Type1Interp::push(double d)
{
  if (_sp < StackSize)
    _s[_sp++] = d;
  else
    _error = errOverflow;
}

inline void
Type1Interp::push_ps(double d)
{
  if (_ps_sp < StackSize)
    _ps_s[_ps_sp++] = d;
  else
    _error = errOverflow;
}

inline double &
Type1Interp::vec(Vector<double> *v, int i)
{
  if (i < 0 || i >= v->count()) {
    _error = errVector;
    return double_for_error;
  } else
    return v->at_u(i);
}

inline Type1Charstring *
Type1Interp::get_subr(int n) const
{
  return _program ? _program->subr(n) : 0;
}

inline Type1Charstring *
Type1Interp::get_glyph(PermString n) const
{
  return _program ? _program->glyph(n) : 0;
}

#endif
