#ifndef T1SICLE_GSUBENCODING_HH
#define T1SICLE_GSUBENCODING_HH
#include <efont/otfgsub.hh>
#include <efont/otfgpos.hh>
class DvipsEncoding;

struct Setting {
    enum { SHOW, KERN, MOVE, RULE };
    int op;
    int x;
    int y;
    Setting(int op_in, int x_in = 0, int y_in = 0)
	: op(op_in), x(x_in), y(y_in) { }
};

class GsubEncoding { public:

    typedef Efont::OpenType::Glyph Glyph;
    typedef Efont::OpenType::Substitution Substitution;
    typedef Efont::OpenType::Positioning Positioning;

    GsubEncoding(int nglyphs);
    // default destructor

    Glyph boundary_glyph() const		{ return _boundary_glyph; }
    Glyph emptyslot_glyph() const		{ return _emptyslot_glyph; }
    String coding_scheme() const		{ return _coding_scheme; }
    void set_coding_scheme(const String &s)	{ _coding_scheme = s; }

    inline Glyph glyph(int) const;
    inline const char *debug_code_name(int) const;
    PermString glyph_name(Glyph, const Vector<PermString> * = 0) const;
    bool setting(int, Vector<Setting> &, bool clear = true) const;
    inline int encoding(Glyph) const;
    int force_encoding(Glyph);

    void encode(int code, Glyph g);

    Glyph add_fake(PermString, const Vector<Setting> &);

    int apply(const Vector<Substitution> &, bool allow_single);
    void apply_substitutions();
    void simplify_ligatures(bool add_fake);
    int apply(const Vector<Positioning> &);
    void simplify_positionings();

    void cut_encoding(int size);
    void shrink_encoding(int size, const DvipsEncoding &, const Vector<PermString> &glyph_names, ErrorHandler *);

    int add_twoligature(int code1, int code2, int outcode, int context = 0);
    int add_threeligature(int code1, int code2, int code3, int outcode);
    void add_kern(int left, int right, int amount);
    void add_single_positioning(int code, int pdx, int pdy, int adx);
    enum { CODE_ALL = 0x7FFFFFFF };
    void remove_ligatures(int code1, int code2);
    void remove_kerns(int code1, int code2);
    void reencode_right_ligkern(int old_code, int new_code);

    int twoligatures(int code1, Vector<int> &code2, Vector<int> &outcode, Vector<int> &context) const;
    int kerns(int code1, Vector<int> &code2, Vector<int> &amount) const;
    int kern(int code1, int code2) const;
    
    bool need_virtual() const		{ return _vfpos.size() > 0 || _fake_names.size() > 0; }
    
    void unparse(const Vector<PermString> * = 0) const;
    
    enum { FIRST_FAKE = 0x10000 };
    
  private:

    Vector<Glyph> _encoding;
    mutable Vector<int> _emap;
    Glyph _boundary_glyph;
    Glyph _emptyslot_glyph;
    String _coding_scheme;

    struct Ligature {
	Vector<int> in;
	int out;
	int skip;
	int context;
	int next;			// not usually valid
	bool live() const		{ return in[0] >= 0; }
	void kill()			{ in[0] = -1; }
	String unparse(const GsubEncoding *) const;
	static inline bool deadp(const Ligature &);
    };
    Vector<Ligature> _ligatures;

    Vector<Setting> _fakes;
    Vector<int> _fake_ptrs;
    Vector<PermString> _fake_names;

    struct Kern {
	int left;
	int right;
	int amount;
    };
    Vector<Kern> _kerns;

    struct Vfpos {
	int in;
	int pdx;
	int pdy;
	int adx;
    };
    Vector<Vfpos> _vfpos;

    int hard_encoding(Glyph) const;
    inline void assign_emap(Glyph, int);
    void add_single_context_substitution(int, int, int, bool is_right);
    static void reassign_ligature(Ligature &, const Vector<int> &);
    void reassign_codes(const Vector<int> &);
    int find_in_place_twoligature(int, int, Vector<int> &, bool add_fake);
    int ligature_score(const Ligature &, Vector<int> &scores) const;
    const Ligature *find_ligature_for(int code) const;
    String unparse_glyph(Glyph, const Vector<PermString> *) const;

    friend bool operator<(const Ligature &, const Ligature &);
    friend bool operator<(const Kern &, const Kern &);
    friend bool operator<(const Vfpos &, const Vfpos &);
    
};

inline GsubEncoding::Glyph
GsubEncoding::glyph(int code) const
{
    if (code < 0 || code >= _encoding.size())
	return 0;
    else
	return _encoding[code];
}

inline int
GsubEncoding::encoding(Glyph g) const
{
    if (g >= 0 && g < _emap.size() && _emap.at_u(g) >= -1)
	return _emap.at_u(g);
    else
	return hard_encoding(g);
}

inline void
GsubEncoding::assign_emap(Glyph g, int code)
{
    if (g >= _emap.size())
	_emap.resize(g + 1, -1);
    _emap[g] = (_emap[g] == -1 || _emap[g] == code ? code : -2);
}

inline bool
operator<(const GsubEncoding::Kern &k1, const GsubEncoding::Kern &k2)
{
    return k1.left < k2.left || (k1.left == k2.left && k1.right < k2.right);
}

inline bool
operator<(const GsubEncoding::Vfpos &p1, const GsubEncoding::Vfpos &p2)
{
    return p1.in < p2.in;
}

#endif
