#ifndef T1SICLE_DVIPSENCODING_HH
#define T1SICLE_DVIPSENCODING_HH
#include <efont/otfcmap.hh>
#include <efont/cff.hh>
#include <lcdf/hashmap.hh>
class GsubEncoding;

class DvipsEncoding { public:

    DvipsEncoding();

    static int parse_glyphlist(String);
    static int glyphname_unicode(const String &);
    static void glyphname_unicode(String, Vector<int> &);

    operator bool() const			{ return _e.size() > 0; }
    const String &name() const			{ return _name; }
    const String &filename() const		{ return _filename; }
    int boundary_char() const			{ return _boundary_char; }
    const String &coding_scheme() const		{ return _coding_scheme; }
    void set_coding_scheme(const String &s)	{ _coding_scheme = s; }

    void encode(int, PermString);
    int encoding_of(PermString) const;
    bool encoded(int e) const;
    int encoding_size() const			{ return _e.size(); }

    void unicodes(Vector<uint32_t> &) const;
    
    int parse(String filename, ErrorHandler *);
    int parse_ligkern(const String &ligkern_text, ErrorHandler *);
    int parse_unicoding(const String &unicoding_text, ErrorHandler *);

    // also modifies 'this':
    void make_literal_gsub_encoding(GsubEncoding &, Efont::Cff::Font *);
    void make_gsub_encoding(GsubEncoding &, const Efont::OpenType::Cmap &, Efont::Cff::Font * = 0);
    
    void apply_ligkern(GsubEncoding &, ErrorHandler *) const;
    
    enum { J_BAD = -1, J_NOKERN = 100, J_NOLIG = 101, J_NOLIGKERN = 102,
	   J_ALL = 0x7FFFFFFF }; // also see nokern_names in dvipsencoding.cc
    
  private:

    struct Ligature {
	int c1, c2, join, d;
    };

    Vector<PermString> _e;
    int _boundary_char;

    Vector<Ligature> _lig;
    HashMap<PermString, int> _unicoding_map;
    Vector<int> _unicoding;

    String _name;
    String _filename;
    String _coding_scheme;
    String _initial_comment;
    String _final_text;

    int parse_ligkern_words(Vector<String> &, ErrorHandler *);
    int parse_unicoding_words(Vector<String> &, ErrorHandler *);
    int parse_words(const String &, int (DvipsEncoding::*)(Vector<String> &, ErrorHandler *), ErrorHandler *);
    void bad_codepoint(int);

    static PermString dot_notdef;

};

inline bool
DvipsEncoding::encoded(int e) const
{
    return e >= 0 && e < _e.size() && _e[e] != dot_notdef;
}

#endif
