#ifndef OTFTOTFM_DVIPSENCODING_HH
#define OTFTOTFM_DVIPSENCODING_HH
#include <efont/otfcmap.hh>
#include <efont/cff.hh>
#include <lcdf/hashmap.hh>
class Metrics;
class Secondary;

class DvipsEncoding { public:

    DvipsEncoding();

    static int parse_glyphlist(String);
    static int glyphname_unicode(const String &, bool *more = 0);
    static void glyphname_unicode(String, Vector<int> &, bool *more = 0);

    operator bool() const			{ return _e.size() > 0; }
    const String &name() const			{ return _name; }
    const String &filename() const		{ return _filename; }
    int boundary_char() const			{ return _boundary_char; }
    const String &coding_scheme() const		{ return _coding_scheme; }
    void set_coding_scheme(const String &s)	{ _coding_scheme = s; }

    void encode(int, PermString);
    inline int encoding_of(PermString) const;
    int encoding_of(PermString, bool encode);
    inline bool encoded(int e) const;
    int encoding_size() const			{ return _e.size(); }

    int parse(String filename, bool ignore_ligkern, bool ignore_other, ErrorHandler *);
    int parse_ligkern(const String &ligkern_text, int override, ErrorHandler *);
    int parse_unicoding(const String &unicoding_text, int override, ErrorHandler *);

    bool file_had_ligkern() const		{ return _file_had_ligkern; }
    
    // also modifies 'this':
    void make_metrics(Metrics &, const Efont::OpenType::Cmap &, Efont::Cff::Font *, Secondary *, bool literal, ErrorHandler *);
    
    void apply_ligkern_lig(Metrics &, ErrorHandler *) const;
    void apply_ligkern_kern(Metrics &, ErrorHandler *) const;
    
    enum { J_BAD = -1,
	   J_LIG = 0, J_CLIG = 1, J_CLIG_S = 2, J_LIGC = 3,
	   J_LIGC_S = 4, J_CLIGC = 5, J_CLIGC_S = 6, J_CLIGC_SS = 7,
	   J_KERN = 100, J_NOLIG = 101, J_NOLIGKERN = 102,
	   J_ALL = 0x7FFFFFFF }; // also see nokern_names in dvipsencoding.cc
    
  private:

    struct Ligature {
	int c1, c2, join, d;
    };

    Vector<PermString> _e;
    Vector<bool> _encoding_required;
    int _boundary_char;
    int _altselector_char;

    Vector<Ligature> _lig;
    HashMap<PermString, int> _unicoding_map;
    Vector<int> _unicoding;

    mutable Vector<uint32_t> _unicodes;

    String _name;
    String _filename;
    String _coding_scheme;
    String _initial_comment;
    String _final_text;
    bool _file_had_ligkern;

    int parse_ligkern_words(Vector<String> &, int override, ErrorHandler *);
    int parse_unicoding_words(Vector<String> &, int override, ErrorHandler *);
    int parse_words(const String &, int override, int (DvipsEncoding::*)(Vector<String> &, int, ErrorHandler *), ErrorHandler *);
    void bad_codepoint(int);
    bool x_unicodes(PermString chname, Vector<uint32_t> &unicodes) const;
    
    static PermString dot_notdef;
    
    friend inline bool operator==(const Ligature&, const Ligature&);

};

inline bool
DvipsEncoding::encoded(int e) const
{
    return e >= 0 && e < _e.size() && _e[e] != dot_notdef;
}

inline int
DvipsEncoding::encoding_of(PermString what) const
{
    return const_cast<DvipsEncoding *>(this)->encoding_of(what, false);
}

#endif
