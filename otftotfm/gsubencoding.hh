#ifndef T1SICLE_GSUBENCODING_HH
#define T1SICLE_GSUBENCODING_HH
#include <efont/otfgsub.hh>
#include <efont/otfgpos.hh>
class DvipsEncoding;

enum { BOUNDARYGLYPH = 0x7FFFFFFFU };

struct Setting {
    enum { SHOW, HMOVETO, VMOVETO };
    int op;
    int x;
    Setting(int op_in, int x_in) : op(op_in), x(x_in) { }
};

class GsubEncoding { public:

    typedef Efont::OpenType::Glyph Glyph;
    typedef Efont::OpenType::Substitution Substitution;
    typedef Efont::OpenType::Positioning Positioning;

    GsubEncoding();
    // default destructor

    int boundary_char() const			{ return _boundary_char; }
    void set_boundary_char(int c)		{ _boundary_char = c; }

    inline Glyph glyph(int) const;
    bool setting(int, Vector<Setting> &) const;
    inline int encoding(Glyph) const;
    int force_encoding(Glyph);
    
    void encode(int code, Glyph g);

    bool apply(const Substitution &, bool allow_single);
    int apply(const Vector<Substitution> &, bool allow_single);
    void apply_substitutions();
    void simplify_ligatures(bool add_fake);
    bool apply(const Positioning &);
    void simplify_kerns();

    void cut_encoding(int size);
    void shrink_encoding(int size, const DvipsEncoding &dvipsenc, const Vector<PermString> &glyph_names, ErrorHandler *);

    void add_twoligature(int code1, int code2, int outcode);
    enum { CODE_ALL = 0x7FFFFFFF };
    void remove_ligatures(int code1, int code2);
    void remove_kerns(int code1, int code2);

    int twoligatures(int code1, Vector<int> &code2, Vector<int> &outcode, Vector<int> &context) const;
    int kerns(int code1, Vector<int> &code2, Vector<int> &amount) const;
    int kern(int code1, int code2) const;
    
    bool need_virtual() const		{ return _vfpos.size() > 0 || _fake_ligatures.size() > 0; }
    
    void unparse(const Vector<PermString> * = 0) const;
    
    enum { FAKE_LIGATURE = 0xFFFF };
    
  private:

    Vector<Glyph> _encoding;
    Vector<Glyph> _substitutions;
    mutable Vector<int> _emap;
    int _boundary_char;

    struct Ligature {
	Vector<int> in;
	int out;
	int skip;
	int context;
    };
    Vector<Ligature> _ligatures;
    Vector<Ligature> _fake_ligatures;

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
    void add_single_rcontext_substitution(int, int, int);
    static void reassign_ligature(Ligature &, const Vector<int> &);
    void reassign_codes(const Vector<int> &);
    int find_skippable_twoligature(int, int, bool add_fake);

    friend bool operator<(const Kern &, const Kern &);
    friend bool operator==(const Kern &, const Kern &);
    
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
operator==(const GsubEncoding::Kern &k1, const GsubEncoding::Kern &k2)
    /* NB: not true equality; useful for remove_kerns, though */
{
    return k1.left == k2.left && k1.right == k2.right;
}

#endif
