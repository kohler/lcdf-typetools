// -*- related-file-name: "../../libefont/otfgsub.cc" -*-
#ifndef EFONT_OTFGSUB_HH
#define EFONT_OTFGSUB_HH
#include <efont/otf.hh>
namespace Efont {

// Have: a list of Unicode characters
// Want: the 'simple' substitutions concerning any of those Unicode characters
// How to do: GlyphIndexSet == set of GlyphIndexes (4bits/12bits)
// Single substitutions
// Generate a list of substitutions of different types that apply?
// Use: ??que?

class OpenTypeSubstitution { public:

    OpenTypeSubstitution();
    OpenTypeSubstitution(const OpenTypeSubstitution &);

    // single substitution
    OpenTypeSubstitution(OpenTypeGlyph in, OpenTypeGlyph out);
    
    // ligature substitution
    OpenTypeSubstitution(OpenTypeGlyph in1, OpenTypeGlyph in2, OpenTypeGlyph out);
    OpenTypeSubstitution(const Vector<OpenTypeGlyph> &in, OpenTypeGlyph out);
    
    ~OpenTypeSubstitution();
    
    OpenTypeSubstitution &operator=(const OpenTypeSubstitution &);

    void unparse(StringAccum &, const Vector<PermString> * = 0) const;
    String unparse(const Vector<PermString> * = 0) const;
    
  private:

    enum { T_NONE = 0, T_GLYPH, T_COVERAGE, T_GLYPHS };
    typedef union {
	OpenTypeGlyph gid;
	OpenTypeGlyph *gids;	// first entry is a count
	OpenTypeCoverage *coverage;
    } Substitute;

    Substitute _left;
    Substitute _in;
    Substitute _out;
    Substitute _right;

    uint8_t _left_is;
    uint8_t _in_is;
    uint8_t _out_is;
    uint8_t _right_is;

    static void clear(Substitute &, uint8_t &);
    static void assign(Substitute &, uint8_t &, OpenTypeGlyph);
    static void assign(Substitute &, uint8_t &, int, const OpenTypeGlyph *);
    static void assign(Substitute &, uint8_t &, const OpenTypeCoverage &);
    static void assign(Substitute &, uint8_t &, const Substitute &, uint8_t);
    
};

class OpenType_GSUB { public:

    OpenType_GSUB(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _str.length() > 0; }

    const OpenTypeScriptList &script_list() const { return _script_list; }
    const OpenTypeFeatureList &feature_list() const { return _feature_list; }

    enum {
	HEADERSIZE = 10,
	L_SINGLE = 1, L_MULTIPLE = 2, L_ALTERNATE = 3, L_LIGATURE = 4,
	L_CONTEXT = 5, L_CHAIN = 6, L_REVCHAIN = 8
    };
    
  private:

    String _str;
    OpenTypeScriptList _script_list;
    OpenTypeFeatureList _feature_list;

    int check(ErrorHandler *);
    
};

class OpenType_GSUBSingle { public:
    OpenType_GSUBSingle(const String &, ErrorHandler * = 0);
    // default destructor
    bool ok() const			{ return _str.length() >= 0; }
    OpenTypeCoverage coverage() const;
    bool covers(OpenTypeGlyph) const;
    OpenTypeGlyph map(OpenTypeGlyph) const;
    enum { HEADERSIZE = 6, FORMAT2_RECSIZE = 2 };
  private:
    String _str;
    int check(ErrorHandler *);
};

class OpenType_GSUBLigature { public:
    OpenType_GSUBLigature(const String &, ErrorHandler * = 0);
    // default destructor
    bool ok() const			{ return _str.length() >= 0; }
    OpenTypeCoverage coverage() const;
    bool covers(OpenTypeGlyph) const;
    bool map(const Vector<OpenTypeGlyph> &, OpenTypeGlyph &, int &) const;
    enum { HEADERSIZE = 6, RECSIZE = 2,
	   SET_HEADERSIZE = 2, SET_RECSIZE = 2,
	   LIG_HEADERSIZE = 4, LIG_RECSIZE = 2 };
  private:
    String _str;
    int check(ErrorHandler *);
};

inline
OpenTypeSubstitution::OpenTypeSubstitution()
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

}
#endif
