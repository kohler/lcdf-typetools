#ifndef OTFTOTFM_METRICS_HH
#define OTFTOTFM_METRICS_HH
#include <efont/otfgsub.hh>
#include <efont/otfgpos.hh>
class DvipsEncoding;

struct Setting {
    enum { NONE, SHOW, KERN, MOVE, RULE, DEAD };
    int op;
    int x;
    int y;
    Setting(int op_in, int x_in = 0, int y_in = 0)
	: op(op_in), x(x_in), y(y_in) { }
    bool valid_op() const		{ return op >= SHOW && op <= RULE; }
};

class Metrics { public:

    typedef int Code;
    typedef Efont::OpenType::Glyph Glyph;
    enum { VIRTUAL_GLYPH = 0x10000 };

    typedef Efont::OpenType::Substitution Substitution;
    typedef Efont::OpenType::Positioning Positioning;

    Metrics(int nglyphs);
    ~Metrics();

    void check() const;
    
    Glyph boundary_glyph() const	{ return _boundary_glyph; }
    Glyph emptyslot_glyph() const	{ return _emptyslot_glyph; }

    String coding_scheme() const		{ return _coding_scheme; }
    void set_coding_scheme(const String &s)	{ _coding_scheme = s; }
    
    inline bool valid_code(Code) const;
    inline bool nonvirtual_code(Code) const;
    PermString code_name(Code, const Vector<PermString> * = 0) const;
    inline const char *code_str(Code, const Vector<PermString> * = 0) const;
    
    inline Glyph glyph(Code) const;
    inline Code encoding(Glyph) const;
    Code force_encoding(Glyph);
    void encode(Code, Glyph);
    void encode_virtual(Code, PermString, const Vector<Setting> &);
    
    void add_ligature(Code in1, Code in2, Code out);
    Code pair_code(Code, Code);
    void add_kern(Code in1, Code in2, int kern);
    void add_single_positioning(Code, int pdx, int pdy, int adx);
    
    enum { CODE_ALL = 0x7FFFFFFF };
    void remove_ligatures(Code in1, Code in2);
    void remove_kerns(Code in1, Code in2);
    void reencode_right_ligkern(Code old_in2, Code new_in2);
    
    int apply(const Vector<Substitution> &, bool allow_single);
    int apply(const Vector<Positioning> &);

    void cut_encoding(int size);
    void shrink_encoding(int size, const DvipsEncoding &, ErrorHandler *);

    bool need_virtual(int size) const;
    bool setting(Code, Vector<Setting> &, bool clear = true) const;
    int ligatures(Code in1, Vector<Code> &in2, Vector<Code> &out, Vector<int> &context) const;
    int kerns(Code in1, Vector<Code> &in2, Vector<int> &kern) const;
    int kern(Code in1, Code in2) const;

    void unparse(const Vector<PermString> *glyph_names = 0) const;

  private:

    struct Ligature {
	Code in2;
	Code out;
	Ligature(Code in2_, Code out_) : in2(in2_), out(out_) { }
    };

    struct Kern {
	Code in2;
	int kern;
	Kern(Code in2_, int kern_) : in2(in2_), kern(kern_) { }
    };

    struct VirtualChar {
	PermString name;
	Vector<Setting> setting;
    };
    
    struct Char {
	Glyph glyph;
	Vector<Ligature> ligatures;
	Vector<Kern> kerns;
	VirtualChar *virtual_char;
	int pdx;
	int pdy;
	int adx;
	int flags;
	enum { CONTEXT = 1, OPEN = 2 };
	Char()			: glyph(0), ligatures(0), virtual_char(0), pdx(0), pdy(0), adx(0), flags(0) { }
	void clear();
	void swap(Char &);
	bool possible_context_setting() const;
	bool context_setting(Code in1, Code in2, const Vector<int> *good = 0) const;
    };

    Vector<Char> _encoding;
    mutable Vector<int> _emap;

    Glyph _boundary_glyph;
    Glyph _emptyslot_glyph;

    String _coding_scheme;
    
    Metrics(const Metrics &);	// does not exist
    Metrics &operator=(const Metrics &); // does not exist

    inline void assign_emap(Glyph, Code);
    Code hard_encoding(Glyph) const;

    Ligature *ligature_obj(Code, Code);
    Kern *kern_obj(Code, Code);
    inline void new_ligature(Code, Code, Code);
    
};


inline bool
Metrics::valid_code(Code code) const
{
    return code >= 0 && code < _encoding.size();
}

inline bool
Metrics::nonvirtual_code(Code code) const
{
    return code >= 0 && code < _encoding.size() && !_encoding[code].virtual_char;
}

inline Metrics::Glyph
Metrics::glyph(Code code) const
{
    if (code < 0 || code >= _encoding.size())
	return 0;
    else
	return _encoding[code].glyph;
}

inline Metrics::Code
Metrics::encoding(Glyph g) const
{
    if (g >= 0 && g < _emap.size() && _emap.at_u(g) >= -1)
	return _emap.at_u(g);
    else
	return hard_encoding(g);
}

inline void
Metrics::assign_emap(Glyph g, Code code)
{
    if (g >= _emap.size())
	_emap.resize(g + 1, -1);
    _emap[g] = (_emap[g] == -1 || _emap[g] == code ? code : -2);
}

inline const char *
Metrics::code_str(Code code, const Vector<PermString> *glyph_names) const
{
    return code_name(code, glyph_names).c_str();
}

#endif
