#ifndef T1CS_HH
#define T1CS_HH
#include "permstr.hh"
#include "string.hh"
#include "vector.hh"
class CharstringInterp;
class EfontMMSpace;
class Type1Encoding;

class Charstring { public:

    Charstring()				{ }
    virtual ~Charstring();

    virtual bool run(CharstringInterp &) const = 0;
    
    enum Commands {
	cError		= 0,
	cHstem		= 1,
	cVstem		= 3,
	cVmoveto	= 4,
	cRlineto	= 5,
	cHlineto	= 6,
	cVlineto	= 7,
	cRrcurveto	= 8,
	cClosepath	= 9,
	cCallsubr	= 10,
	cReturn		= 11,
	cEscape		= 12,
	cHsbw		= 13,
	cEndchar	= 14,
	cBlend		= 16,
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
    
	cEscapeDelta	= 32,
	cDotsection	= 32 + 0,
	cVstem3		= 32 + 1,
	cHstem3		= 32 + 2,
	cAnd		= 32 + 3,
	cOr		= 32 + 4,
	cNot		= 32 + 5,
	cSeac		= 32 + 6,
	cSbw		= 32 + 7,
	cStore		= 32 + 8,
	cAbs		= 32 + 9,
	cAdd		= 32 + 10,
	cSub		= 32 + 11,
	cDiv		= 32 + 12,
	cLoad		= 32 + 13,
	cNeg		= 32 + 14,
	cEq		= 32 + 15,
	cCallothersubr	= 32 + 16,
	cPop		= 32 + 17,
	cDrop		= 32 + 18,
	cPut		= 32 + 20,
	cGet		= 32 + 21,
	cIfelse		= 32 + 22,
	cRandom		= 32 + 23,
	cMul		= 32 + 24,
	cSqrt		= 32 + 26,
	cDup		= 32 + 27,
	cExch		= 32 + 28,
	cIndex		= 32 + 29,
	cRoll		= 32 + 30,
	cSetcurrentpoint = 32 + 33,
	cHflex		= 32 + 34,
	cFlex		= 32 + 35,
	cHflex1		= 32 + 36,
	cFlex1		= 32 + 37,

	cLastCommand	= cFlex1
    };

    enum OthersubrCommands {
	othcFlexend = 0,
	othcFlexbegin = 1,
	othcFlexmiddle = 2,
	othcReplacehints = 3,
	othcMM1 = 14,
	othcMM2 = 15,
	othcMM3 = 16,
	othcMM4 = 17,
	othcMM6 = 18,
	othcITC_load = 19,
	othcITC_add = 20,
	othcITC_sub = 21,
	othcITC_mul = 22,
	othcITC_div = 23,
	othcITC_put = 24,
	othcITC_get = 25,
	othcITC_unknown = 26,
	othcITC_ifelse = 27,
	othcITC_random = 28
    };

    static String command_name(int);
    static const char * const command_names[];
    
};


class Type1Charstring : public Charstring { public:
    
    Type1Charstring()				{ }
    Type1Charstring(const String &);		// unencrypted
    Type1Charstring(int lenIV, const String &);	// encrypted
    Type1Charstring(const Type1Charstring &);
    ~Type1Charstring()				{ }
  
    const unsigned char *data() const;
    int length() const				{ return _s.length(); }
  
    void assign(const String &);
    void prepend(const Type1Charstring &);
    
    bool run(CharstringInterp &) const;

  private:

    mutable String _s;
    mutable int _key;
  
    Type1Charstring &operator=(const Type1Charstring &);
  
    void decrypt() const;
    
};


class Type2Charstring : public Charstring { public:
  
    Type2Charstring()				{ }
    Type2Charstring(const String &);
    Type2Charstring(const Type2Charstring &);
    ~Type2Charstring()				{ }
  
    const unsigned char *data() const;
    int length() const				{ return _s.length(); }
    
    bool run(CharstringInterp &) const;

  private:
  
    String _s;
  
    Type2Charstring &operator=(const Type2Charstring &);

};


class EfontProgram { public:
  
    EfontProgram()					{ }
    virtual ~EfontProgram()				{ }

    virtual PermString font_name() const		{ return PermString();}
    
    virtual int nsubrs() const				{ return 0; }
    virtual Charstring *subr(int) const			{ return 0; }
    virtual int ngsubrs() const				{ return 0; }
    virtual Charstring *gsubr(int) const		{ return 0; }
    
    virtual int nglyphs() const				{ return 0; }
    virtual PermString glyph_name(int) const		{ return PermString();}
    virtual void glyph_names(Vector<PermString> &) const;
    virtual Charstring *glyph(int) const		{ return 0; }
    virtual Charstring *glyph(PermString) const		{ return 0; }

    virtual bool is_mm() const				{ return mmspace(); }
    virtual EfontMMSpace *mmspace() const		{ return 0; }
    virtual Vector<double> *design_vector() const	{ return 0; }
    virtual Vector<double> *norm_design_vector() const	{ return 0; }
    virtual Vector<double> *weight_vector() const	{ return 0; }
    virtual bool writable_vectors() const		{ return false; }

    virtual Type1Encoding *type1_encoding() const	{ return 0; }

    virtual double global_width_x(bool is_nominal) const;
    
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
