#ifndef T1MM_HH
#define T1MM_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "vector.hh"
#include "permstr.hh"
class Type1Charstring;
class ErrorHandler;
class Type1Font;

class Type1MMSpace {
  
  typedef Vector<double> NumVector;
  
  mutable bool _ok;
  
  PermString _font_name;
  int _naxes;
  int _nmasters;
  
  Vector<NumVector> _master_positions;
  Vector<NumVector> _normalize_in;
  Vector<NumVector> _normalize_out;
  
  Vector<PermString> _axis_types;
  Vector<PermString> _axis_labels;
  
  bool _own_ndv;
  bool _own_cdv;
  Type1Charstring *_ndv;
  Type1Charstring *_cdv;
  
  NumVector _design_vector;
  NumVector _weight_vector;
  
  bool set_error(ErrorHandler *, const char *, ...) const;
  
  bool normalize_vector(NumVector &, NumVector &, NumVector &,
			ErrorHandler *) const;
  bool convert_vector(NumVector &, NumVector &, NumVector &,
		      ErrorHandler *) const;
  
 public:
  
  Type1MMSpace(PermString, int naxes, int nmasters);
  ~Type1MMSpace();

  PermString font_name() const		{ return _font_name; }
  
  int axis(PermString) const;
  double axis_low(int) const;
  double axis_high(int) const;
  
  PermString axis_type(int a) const	{ return _axis_types[a]; }
  PermString axis_label(int a) const	{ return _axis_labels[a]; }

  Type1Charstring *ndv() const		{ return _ndv; }
  Type1Charstring *cdv() const		{ return _cdv; }

  void set_master_positions(const Vector<NumVector> &);
  void set_normalize(const Vector<NumVector> &, const Vector<NumVector> &);
  void set_axis_type(int, PermString);
  void set_axis_label(int, PermString);
  void set_ndv(Type1Charstring *, bool own = false);
  void set_cdv(Type1Charstring *, bool own = false);
  void set_design_vector(const NumVector &);
  void set_weight_vector(const NumVector &);
  
  bool check(ErrorHandler * = 0);
  
  NumVector design_vector() const;
  bool set_design(NumVector &, int, double, ErrorHandler * = 0) const;
  bool set_design(NumVector &, PermString, double, ErrorHandler * = 0) const;
  
  bool norm_design_vector(const NumVector &, NumVector &,
			  ErrorHandler * = 0) const; 
  bool weight_vector(const NumVector &, NumVector &,
		     ErrorHandler * = 0) const; 
  
  static Type1MMSpace *create(const Type1Font *, ErrorHandler * = 0);
  
};

#endif
