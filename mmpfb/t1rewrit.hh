#ifndef T1REWRIT_HH
#define T1REWRIT_HH
#include "t1interp.hh"
#include "t1font.hh"
#include "straccum.hh"

class Type1CharstringGen {
  
  StringAccum _ncs;
  int _precision;
  
 public:
  
  Type1CharstringGen(int precision = 5);

  void clear()				{ _ncs.clear(); }
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
  
  int _subr_count;
  Vector<int> _subr_done;
  Vector<Type1Charstring *> _subr_prefix;
  Vector<int> _subr_contains_mm;
  
  bool _contains_mm_warned;
  ErrorHandler *_errh;
  
 public:
  
  Type1MMRemover(Type1Font *, Vector<double> *, int, ErrorHandler *);
  ~Type1MMRemover();
  
  Type1Program *program() const		{ return _font; }
  Vector<double> *weight_vector() const	{ return _weight_vector; }
  int nmasters() const			{ return _weight_vector->size(); }
  int precision() const			{ return _precision; }
  
  Type1Charstring *subr_prefix(int);
  Type1Charstring *subr(int);
  bool subr_empty(int);
  bool subr_contains_mm(int);
  
  void run();
  
};


class Type1OneMMRemover: public Type1Interp {
  
  Type1MMRemover *_remover;
  Type1CharstringGen _prefix_gen;
  Type1CharstringGen _main_gen;
  
  int _subr_level;
  bool _in_prefix;
  bool _reduce;
  bool _output_mm;
  
  void run_subr(Type1Charstring *);
  bool run_once(const Type1Charstring &, bool do_prefix);
  
 public:
  
  Type1OneMMRemover(Type1MMRemover *);
  
  bool command(int);
  
  void run(const Type1Charstring &, bool do_prefix);

  Type1Charstring *output_prefix();
  void output_main(Type1Charstring &);
  bool contained_mm() const			{ return _output_mm; }
  
};

#endif
