#ifndef MYFONT_HH
#define MYFONT_HH
#include "t1font.hh"
#include <stdio.h>
class Type1MMSpace;
class ErrorHandler;


class MyFont: public Type1Font {
  
  typedef Vector<double> NumVector;

  int _nmasters;
  Vector<double> _weight_vector;
  
  void interpolate_dict_numvec(PermString, bool = true, bool = false);
  void interpolate_dict_num(PermString, bool = true);
  
 public:
  
  MyFont(Type1Reader &);
  ~MyFont();
  
  bool set_design_vector(Type1MMSpace *, const Vector<double> &,
			 ErrorHandler * = 0);
  
  void interpolate_dicts(ErrorHandler * = 0);
  void interpolate_charstrings(ErrorHandler * = 0);
  
};

#endif
