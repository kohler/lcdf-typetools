#ifndef OTFTOTFM_SECONDARY_HH
#define OTFTOTFM_SECONDARY_HH
#include <efont/otfcmap.hh>
#include <efont/cff.hh>
class Metrics;
class Transform;
struct Setting;

class Secondary { public:
    Secondary()				: _next(0) { }
    virtual ~Secondary();
    void set_next(Secondary *s)		{ _next = s; }
    typedef Efont::OpenType::Glyph Glyph;
    virtual bool encode_uni(int code, PermString name, uint32_t uni, Metrics &, ErrorHandler *);
    virtual bool setting(uint32_t uni, Vector<Setting> &, Metrics &, ErrorHandler *);
  private:
    Secondary *_next;
};

class T1Secondary : public Secondary { public:
    T1Secondary(const Efont::Cff::Font *, const Efont::OpenType::Cmap &);
    void set_font_information(const String &font_name, const Efont::OpenType::Font &, const String &otf_file_name);
    bool encode_uni(int code, PermString name, uint32_t uni, Metrics &, ErrorHandler *);
    bool setting(uint32_t uni, Vector<Setting> &, Metrics &, ErrorHandler *);
  private:
    const Efont::OpenType::Font *_otf;
    const Efont::Cff::Font *_cff;
    String _font_name;
    String _otf_file_name;
    int _xheight;
    int _spacewidth;
    bool two_char_setting(uint32_t uni1, uint32_t uni2, Vector<Setting> &, Metrics &);
    enum { J_NODOT = -1031892 /* unlikely value */ };
    int dotlessj_font(Metrics &, ErrorHandler *, Glyph &dj_glyph);
};

bool char_bounds(int bounds[4], int &width,
		 const Efont::Cff::Font *, const Efont::OpenType::Cmap &,
		 uint32_t uni, const Transform &);

#endif
