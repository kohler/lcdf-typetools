#ifndef T1REWRIT_HH
#define T1REWRIT_HH
#include <efont/t1interp.hh>
#include <efont/t1font.hh>
#include <lcdf/straccum.hh>

class Type1CharstringGen { public:

    Type1CharstringGen(int precision = 5);

    void clear();
    char *data() const			{ return _ncs.data(); }
    int length() const			{ return _ncs.length(); }

    void gen_number(double, int = 0);
    void gen_command(int);
    void gen_stack(Efont::CharstringInterp &, int for_cmd);

    Efont::Type1Charstring *output();
    void output(Efont::Type1Charstring &);

  private:
  
    StringAccum _ncs;
    int _precision;
    double _f_precision;
    
    double _true_x;
    double _true_y;
    double _false_x;
    double _false_y;
  
};


class Type1MMRemover {
  
  Efont::Type1Font *_font;
  Vector<double> *_weight_vector;
  int _precision;
  
  int _nsubrs;
  Vector<int> _subr_done;
  Vector<Efont::Type1Charstring *> _subr_prefix;
  Vector<int> _must_expand_subr;
  Vector<int> _hint_replacement_subr;
  bool _expand_all_subrs;
 
  ErrorHandler *_errh;
  
 public:
  
  Type1MMRemover(Efont::Type1Font *, Vector<double> *, int, ErrorHandler *);
  ~Type1MMRemover();
  
  Efont::EfontProgram *program() const	{ return _font; }
  Vector<double> *weight_vector() const	{ return _weight_vector; }
  int nmasters() const			{ return _weight_vector->size(); }
  int precision() const			{ return _precision; }
  
  Efont::Type1Charstring *subr_prefix(int);
  Efont::Type1Charstring *subr_expander(int);
  
  void run();
  
};


class Type1SubrRemover { public:
  
    Type1SubrRemover(Efont::Type1Font *, ErrorHandler *);
    ~Type1SubrRemover();
  
    Efont::EfontProgram *program() const	{ return _font; }
    ErrorHandler *errh() const			{ return _errh; }
  
    int save_count() const			{ return _save_count; }
  
    bool run(int);
    
  private:
  
    Efont::Type1Font *_font;
  
    int _nsubrs;
    enum { REMOVABLE = -1, DEAD = -2 };
    Vector<int> _renumbering;
    Vector<int> _cost;
    int _save_count;
    int _nonexist_count;
  
    ErrorHandler *_errh;

};


#endif
