#ifndef T1CS_HH
#define T1CS_HH
#include "permstr.hh"
#include "vector.hh"
class Type1Interp;

class Type1Charstring {
  
  unsigned char *_data;
  int _len;
  mutable int _key;
  
  Type1Charstring &operator=(const Type1Charstring &);
  
  void decrypt() const;
  
 public:
  
  Type1Charstring()				: _data(0), _len(0) { }
  Type1Charstring(unsigned char *, int);	// unencrypted, takes data
  Type1Charstring(int, unsigned char *, int);	// encrypted, copies data
  Type1Charstring(const Type1Charstring &);
  ~Type1Charstring()				{ delete[] _data; }
  
  unsigned char *data() const;
  int length() const				{ return _len; }
  
  void assign(unsigned char *, int);
  
  bool run(Type1Interp &) const;
  
};


class Type1Program {
  
 public:
  
  Type1Program()					{ }
  virtual ~Type1Program()				{ }
  
  virtual Type1Charstring *subr(int) const		{ return 0; }
  virtual Type1Charstring *glyph(PermString) const	{ return 0; }
  
  virtual Vector<double> *design_vector() const		{ return 0; }
  virtual Vector<double> *norm_design_vector() const	{ return 0; }
  virtual Vector<double> *weight_vector() const		{ return 0; }
  virtual bool writable_vectors() const			{ return false; }
  
};


enum Type1Defs {
  t1Warmup_ee	= 4,
  t1R_ee	= 55665,
  t1R_cs	= 4330,
  t1C1		= 52845,
  t1C2		= 22719,
};


inline
Type1Charstring::Type1Charstring(unsigned char *d, int l)
  : _data(d), _len(l), _key(-1)
{
}

inline void
Type1Charstring::assign(unsigned char *d, int l)
{
  delete[] _data;
  _data = d;
  _len = l;
  _key = -1;
}

inline unsigned char *
Type1Charstring::data() const
{
  if (_key >= 0) decrypt();
  return _data;
}

#endif
