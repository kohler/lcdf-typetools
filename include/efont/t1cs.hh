#ifndef T1CS_HH
#define T1CS_HH
#include "permstr.hh"
#include "string.hh"
#include "vector.hh"
class Type1Interp;
class PsfontMMSpace;
class Type1Encoding;

class PsfontCharstring { public:

    PsfontCharstring()				{ }
    virtual ~PsfontCharstring();

    virtual bool run(Type1Interp &) const = 0;
    
};


class Type1Charstring : public PsfontCharstring { public:
  
    Type1Charstring()				{ }
    Type1Charstring(const String &);		// unencrypted
    Type1Charstring(int lenIV, const String &);	// encrypted
    Type1Charstring(const Type1Charstring &);
    ~Type1Charstring()				{ }
  
    const unsigned char *data() const;
    int length() const				{ return _s.length(); }
  
    void assign(const String &);
    void prepend(const Type1Charstring &);
    
    bool run(Type1Interp &) const;

  private:

    mutable String _s;
    mutable int _key;
  
    Type1Charstring &operator=(const Type1Charstring &);
  
    void decrypt() const;
    
};


class Type2Charstring : public PsfontCharstring { public:
  
    Type2Charstring()				{ }
    Type2Charstring(const String &);
    Type2Charstring(const Type2Charstring &);
    ~Type2Charstring()				{ }
  
    const unsigned char *data() const;
    int length() const				{ return _s.length(); }
    
    bool run(Type1Interp &) const;

  private:
  
    String _s;
  
    Type2Charstring &operator=(const Type1Charstring &);

};


class PsfontProgram { public:
  
    PsfontProgram()					{ }
    virtual ~PsfontProgram()				{ }

    virtual PermString font_name() const		{ return PermString();}
    
    virtual int nsubrs() const				{ return 0; }
    virtual PsfontCharstring *subr(int) const		{ return 0; }
    virtual int ngsubrs() const				{ return 0; }
    virtual PsfontCharstring *gsubr(int) const		{ return 0; }
    
    virtual int nglyphs() const				{ return 0; }
    virtual PermString glyph_name(int) const		{ return PermString();}
    virtual void glyph_names(Vector<PermString> &) const;
    virtual PsfontCharstring *glyph(int) const		{ return 0; }
    virtual PsfontCharstring *glyph(PermString) const	{ return 0; }

    virtual bool is_mm() const				{ return mmspace(); }
    virtual PsfontMMSpace *mmspace() const		{ return 0; }
    virtual Vector<double> *design_vector() const	{ return 0; }
    virtual Vector<double> *norm_design_vector() const	{ return 0; }
    virtual Vector<double> *weight_vector() const	{ return 0; }
    virtual bool writable_vectors() const		{ return false; }

    virtual Type1Encoding *type1_encoding() const	{ return 0; }
    
};


enum Type1Defs {
    t1Warmup_ee	= 4,
    t1R_ee	= 55665,
    t1R_cs	= 4330,
    t1C1	= 52845,
    t1C2	= 22719,
};


inline
Type1Charstring::Type1Charstring(const String &s)
    : _s(s), _key(-1)
{
}

inline void
Type1Charstring::assign(const String &s)
{
    _s = s;
    _key = -1;
}

inline const unsigned char *
Type1Charstring::data() const
{
    if (_key >= 0)
	decrypt();
    return reinterpret_cast<const unsigned char *>(_s.data());
}


inline
Type2Charstring::Type2Charstring(const String &s)
    : _s(s)
{
}

inline const unsigned char *
Type2Charstring::data() const
{
    return reinterpret_cast<const unsigned char *>(_s.data());
}

#endif
