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
    
    int encoding_of(const String &) const;

    int parse(const String &, ErrorHandler *);

    void make_gsub_encoding(GsubEncoding &, const Efont::OpenType::Cmap &, Efont::EfontCFF::Font * = 0);
    
  private:

    enum { J_NOKERN = 100, J_ALL = 0x7FFFFFFF };
    
    struct Ligature {
	int c1, c2, join, d;
    };

    String _name;
    Vector<PermString> _e;
    String _initial_comment;
    String _final_text;
    int _boundary_char;

    Vector<Ligature> _lig;
    HashMap<PermString, int> _unicoding_map;
    Vector<int> _unicoding;

    int parse_ligkern(const Vector<String> &);
    int parse_unicoding(const Vector<String> &);
    int parse_words(const String &, int (DvipsEncoding::*)(const Vector<String> &));

};

#endif
