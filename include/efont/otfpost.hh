// -*- related-file-name: "../../libefont/otfpost.cc" -*-
#ifndef EFONT_OTFPOST_HH
#define EFONT_OTFPOST_HH
#include <efont/otf.hh>
#include <lcdf/error.hh>
namespace Efont { namespace OpenType {

class Post { public:

    Post(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

    double italic_angle() const;
    bool is_fixed_pitch() const;
    int nglyphs() const			{ return _nglyphs; }
    bool glyph_names(Vector<PermString> &gnames) const;

  private:

    String _str;
    int _error;
    uint32_t _version;
    int _nglyphs;
    Vector<int> _extend_glyph_names;

    enum { HEADER_SIZE = 32, N_MAC_GLYPHS = 258 };
    int parse_header(ErrorHandler *);
    
};

}}
#endif
