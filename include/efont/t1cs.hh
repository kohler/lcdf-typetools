#ifndef T1CS_HH
#define T1CS_HH
#include "permstr.hh"
#include "vector.hh"
class Type1Interp;

class PsfontCharstring { public:

    PsfontCharstring()				{ }
    virtual ~PsfontCharstring();

    virtual bool run(Type1Interp &) const = 0;
    
};


class Type1Charstring : public PsfontCharstring { public:
  
    Type1Charstring()				: _data(0), _len(0) { }
    Type1Charstring(unsigned char *, int);	// unencrypted, takes data
    Type1Charstring(int lenIV, unsigned char *, int); // encrypted, copies data
    Type1Charstring(const Type1Charstring &);
    ~Type1Charstring()				{ delete[] _data; }
  
    const unsigned char *data() const;
    int length() const				{ return _len; }
  
    void assign(unsigned char *, int);
    void prepend(const Type1Charstring &);
    
    bool run(Type1Interp &) const;

  private:
  
    unsigned char *_data;
    int _len;
    mutable int _key;
  
    Type1Charstring &operator=(const Type1Charstring &);
  
    void decrypt() const;
    
};


class Type2Charstring : public PsfontCharstring { public:
  
    Type2Charstring()				: _data(0), _len(0) { }
    Type2Charstring(unsigned char *, int);	// takes data
    Type2Charstring(const Type2Charstring &);
    ~Type2Charstring()				{ delete[] _data; }
  
    const unsigned char *data() const		{ return _data; }
    int length() const				{ return _len; }
    
    bool run(Type1Interp &) const;

  private:
  
    unsigned char *_data;
    int _len;
  
    Type2Charstring &operator=(const Type1Charstring &);

};


class PsfontProgram { public:
  
    PsfontProgram()					{ }
    virtual ~PsfontProgram()				{ }

    virtual int nsubrs() const				{ return 0; }
    virtual PsfontCharstring *subr(int) const		{ return 0; }
    virtual PsfontCharstring *glyph(PermString) const	{ return 0; }
  
    virtual Vector<double> *design_vector() const	{ return 0; }
    virtual Vector<double> *norm_design_vector() const	{ return 0; }
    virtual Vector<double> *weight_vector() const	{ return 0; }
    virtual bool writable_vectors() const		{ return false; }
  
};


enum Type1Defs {
    t1Warmup_ee	= 4,
    t1R_ee	= 55665,
    t1R_cs	= 4330,
    t1C1	= 52845,
    t1C2	= 22719,
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

inline const unsigned char *
Type1Charstring::data() const
{
    if (_key >= 0)
	decrypt();
    return _data;
}


inline
Type2Charstring::Type2Charstring(unsigned char *d, int l)
    : _data(d), _len(l)
{
}

#endif
