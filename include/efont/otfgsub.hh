// -*- related-file-name: "../../libefont/otfgsub.cc" -*-
#ifndef EFONT_OTFGSUB_HH
#define EFONT_OTFGSUB_HH
#include <efont/otf.hh>
#include <efont/otfdata.hh>
namespace Efont { namespace OpenType {
class GsubLookup;
class Substitution;

class Gsub { public:

    Gsub(const Data &, ErrorHandler * = 0) throw (Error);
    // default destructor

    const ScriptList &script_list() const { return _script_list; }
    const FeatureList &feature_list() const { return _feature_list; }

    GsubLookup lookup(unsigned) const;

    enum { HEADERSIZE = 10 };
    
  private:

    ScriptList _script_list;
    FeatureList _feature_list;
    Data _lookup_list;
    
};

class GsubLookup { public:
    GsubLookup(const Data &) throw (Error);
    int type() const			{ return _d.u16(0); }
    uint16_t flags() const		{ return _d.u16(2); }
    void unparse_automatics(Vector<Substitution> &) const;
    enum {
	HEADERSIZE = 6, RECSIZE = 2,
	L_SINGLE = 1, L_MULTIPLE = 2, L_ALTERNATE = 3, L_LIGATURE = 4,
	L_CONTEXT = 5, L_CHAIN = 6, L_REVCHAIN = 8
    };
  private:
    Data _d;
};

class GsubSingle { public:
    GsubSingle(const Data &) throw (Error);
    // default destructor
    Coverage coverage() const throw ();
    Glyph map(Glyph) const;
    void unparse(Vector<Substitution> &) const;
    enum { HEADERSIZE = 6, FORMAT2_RECSIZE = 2 };
  private:
    Data _d;
};

class GsubMultiple { public:
    GsubMultiple(const Data &) throw (Error);
    // default destructor
    Coverage coverage() const throw ();
    bool map(Glyph, Vector<Glyph> &) const;
    void unparse(Vector<Substitution> &, bool alternate = false) const;
    enum { HEADERSIZE = 6, RECSIZE = 2,
	   SEQ_HEADERSIZE = 2, SEQ_RECSIZE = 2 };
  private:
    Data _d;
};

class GsubLigature { public:
    GsubLigature(const Data &) throw (Error);
    // default destructor
    Coverage coverage() const throw ();
    bool map(const Vector<Glyph> &, Glyph &, int &) const;
    void unparse(Vector<Substitution> &) const;
    enum { HEADERSIZE = 6, RECSIZE = 2,
	   SET_HEADERSIZE = 2, SET_RECSIZE = 2,
	   LIG_HEADERSIZE = 4, LIG_RECSIZE = 2 };
  private:
    Data _d;
};

class Substitution { public:

    Substitution();
    Substitution(const Substitution &);

    // single substitution
    Substitution(Glyph in, Glyph out);
    
    // multiple substitution
    Substitution(Glyph in, const Vector<Glyph> &out, bool is_alternate = false);
    
    // ligature substitution
    Substitution(Glyph in1, Glyph in2, Glyph out);
    Substitution(const Vector<Glyph> &in, Glyph out);
    
    ~Substitution();
    
    Substitution &operator=(const Substitution &);

    bool context_in(const Coverage &) const;
    bool context_in(const GlyphSet &) const;

    // types
    operator bool() const;
    bool is_single() const;
    bool is_multiple() const;
    bool is_alternate() const;
    bool is_ligature() const;

    // extract data
    Glyph in_glyph() const;
    bool in_glyphs(Vector<Glyph> &) const;
    Glyph out_glyph() const;
    bool out_glyphs(Vector<Glyph> &) const;
    
    void unparse(StringAccum &, const Vector<PermString> * = 0) const;
    String unparse(const Vector<PermString> * = 0) const;
    
  private:

    enum { T_NONE = 0, T_GLYPH, T_COVERAGE, T_GLYPHS };
    typedef union {
	Glyph gid;
	Glyph *gids;	// first entry is a count
	Coverage *coverage;
    } Substitute;

    Substitute _left;
    Substitute _in;
    Substitute _out;
    Substitute _right;

    uint8_t _left_is;
    uint8_t _in_is;
    uint8_t _out_is;
    uint8_t _right_is;

    bool _alternate : 1;

    static void clear(Substitute &, uint8_t &);
    static void assign(Substitute &, uint8_t &, Glyph);
    static void assign(Substitute &, uint8_t &, int, const Glyph *);
    static void assign(Substitute &, uint8_t &, const Coverage &);
    static void assign(Substitute &, uint8_t &, const Substitute &, uint8_t);
    static bool substitute_in(const Substitute &, uint8_t, const Coverage &);
    static bool substitute_in(const Substitute &, uint8_t, const GlyphSet &);

    static Glyph extract_glyph(const Substitute &, uint8_t);
    static bool extract_glyphs(const Substitute &, uint8_t, Vector<Glyph> &);
    
};

inline
Substitution::Substitution()
    : _left_is(T_NONE), _in_is(T_NONE), _out_is(T_NONE), _right_is(T_NONE)
{
}

/* Single 1: u16 format, offset coverage, u16 glyphdelta
   Single 2: u16 format, offset coverage, u16 count, glyph subst[]
   Multiple 1: u16 format, offset coverage, u16 count, offset sequence[];
     sequence is: u16 count, glyph subst[]
   Alternate 1: u16 format, offset coverage, u16 count, offset alternates[];
     alternate is: u16 count, glyph alts[]
   Ligature 1: u16 format, offset coverage, u16 count, offset sets[];
     set is: u16 count, offset ligatures[];
     ligature is: glyph result, u16 count, glyph components[]
*/

inline
Substitution::operator bool() const
{
    return !(_left_is == T_NONE && _in_is == T_NONE && _out_is == T_NONE && _right_is == T_NONE);
}

inline bool
Substitution::is_single() const
{
    return _left_is == T_NONE && _in_is == T_GLYPH && _out_is == T_GLYPH && _right_is == T_NONE;
}

inline bool
Substitution::is_multiple() const
{
    return _left_is == T_NONE && _in_is == T_GLYPH && _out_is == T_GLYPHS && _right_is == T_NONE && !_alternate;
}

inline bool
Substitution::is_alternate() const
{
    return _left_is == T_NONE && _in_is == T_GLYPH && _out_is == T_GLYPHS && _right_is == T_NONE && _alternate;
}

inline bool
Substitution::is_ligature() const
{
    return _left_is == T_NONE && _in_is == T_GLYPHS && _out_is == T_GLYPH && _right_is == T_NONE;
}

inline Glyph
Substitution::in_glyph() const
{
    return extract_glyph(_in, _in_is);
}

inline bool
Substitution::in_glyphs(Vector<Glyph> &v) const
{
    return extract_glyphs(_in, _in_is, v);
}

inline Glyph
Substitution::out_glyph() const
{
    return extract_glyph(_out, _out_is);
}

inline bool
Substitution::out_glyphs(Vector<Glyph> &v) const
{
    return extract_glyphs(_out, _out_is, v);
}

}}
#endif
