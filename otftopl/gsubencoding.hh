#ifndef T1SICLE_GSUBENCODING_HH
#define T1SICLE_GSUBENCODING_HH
#include <efont/otfgsub.hh>

class GsubEncoding { public:

    typedef Efont::OpenType::Glyph Glyph;
    typedef Efont::OpenType::Substitution Substitution;

    GsubEncoding();
    // default destructor

    Glyph glyph(int) const;
    int encoding(Glyph) const;
    int force_encoding(Glyph);
    
    void encode(int code, Glyph g);

    void apply(const Substitution &);
    void apply_substitutions();

    void simplify_ligatures(bool add_fake);
    void shrink_encoding(int size);

    void unparse(const Vector<PermString> * = 0) const;
    
    enum { FAKE_LIGATURE = 0xFFFF };
    
  private:

    Vector<Glyph> _encoding;
    Vector<Glyph> _substitutions;

    struct Ligature {
	Vector<int> in;
	int out;
	int skip;
    };
    Vector<Ligature> _ligatures;
    Vector<Ligature> _fake_ligatures;
    
    void apply_single_substitution(Glyph, Glyph);
    static void reassign_ligature(Ligature &, const Vector<int> &);
    int find_skippable_twoligature(int, int, bool add_fake);
    
};

inline GsubEncoding::Glyph
GsubEncoding::glyph(int code) const
{
    if (code < 0 || code >= _encoding.size())
	return 0;
    else
	return _encoding[code];
}

#endif
