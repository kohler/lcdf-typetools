#ifndef T1REWRIT_HH
#define T1REWRIT_HH
#include "t1interp.hh"
#include "t1font.hh"
#include "straccum.hh"

class Type1CharstringGen {
  
    StringAccum _ncs;
    int _precision;
    double _f_precision;
    double _f_rounder;
  
  public:
  
    Type1CharstringGen(int precision = 5);

    void clear()			{ _ncs.clear(); }
    char *data() const			{ return _ncs.data(); }
    int length() const			{ return _ncs.length(); }

    void gen_number(double);
    void gen_command(int);
    void gen_stack(Type1Interp &);

    Type1Charstring *output();
    void output(Type1Charstring &);

};


class Type1MMRemover {
  
  Type1Font *_font;
  Vector<double> *_weight_vector;
  int _precision;
  
  int _nsubrs;
  Vector<int> _subr_done;
  Vector<Type1Charstring *> _subr_prefix;
  Vector<int> _must_expand_subr;
  Vector<int> _hint_replacement_subr;
  bool _expand_all_subrs;
 
  ErrorHandler *_errh;
  
 public:
  
  Type1MMRemover(Type1Font *, Vector<double> *, int, ErrorHandler *);
  ~Type1MMRemover();
  
  Type1Program *program() const		{ return _font; }
  Vector<double> *weight_vector() const	{ return _weight_vector; }
  int nmasters() const			{ return _weight_vector->size(); }
  int precision() const			{ return _precision; }
  
  Type1Charstring *subr_prefix(int);
  Type1Charstring *subr_expander(int);
  
  void run();
  
};


class Type1SubrRemover {
  
  Type1Font *_font;
  
  int _nsubrs;
  Vector<bool> _save;
  Vector<int> _cost;
  int _save_count;
  int _nonexist_count;
  
  ErrorHandler *_errh;
  
 public:
  
  Type1SubrRemover(Type1Font *, ErrorHandler *);
  ~Type1SubrRemover();
  
  Type1Program *program() const			{ return _font; }
  ErrorHandler *errh() const			{ return _errh; }
  
  void mark_save(int n);
  int save_count() const			{ return _save_count; }
  
  bool run(int);
  
};


inline void
Type1SubrRemover::mark_save(int n)
{
  if (n >= 0 && n < _nsubrs && !_save[n]) {
    _save[n] = true;
    _save_count++;
  }
}

#endif
