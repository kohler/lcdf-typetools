#ifndef T1CS_HH
#define T1CS_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "permstr.hh"
class Type1Interp;

class Type1Charstring {
  
  unsigned char *_data;
  int _len;
  int _key;
  
  Type1Charstring(const Type1Charstring &);
  Type1Charstring &operator=(const Type1Charstring &);

  void decrypt();
  
 public:
  
  Type1Charstring(unsigned char *, int);	// unencrypted, takes data
  Type1Charstring(int, unsigned char *, int);	// encrypted, copies data
  Type1Charstring()				: _data(0), _len(0) { }
  ~Type1Charstring()				{ delete[] _data; }
  
  unsigned char *data();
  int length() const				{ return _len; }
  
  void assign(unsigned char *, int);
  
  bool run(Type1Interp &);
  
};


class Type1Program {
  
 public:
  
  Type1Program()					{ }
  virtual ~Type1Program()				{ }
  
  virtual Type1Charstring *subr(int) const		{ return 0; }
  virtual Type1Charstring *glyph(PermString) const	{ return 0; }
  
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

enum Type1Defs {
  t1Warmup_ee	= 4,
  t1R_ee	= 55665,
  t1R_cs	= 4330,
  t1C1		= 52845,
  t1C2		= 22719,
};

#endif
