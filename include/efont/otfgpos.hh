// -*- related-file-name: "../../libefont/otfgpos.cc" -*-
#ifndef EFONT_OTFGPOS_HH
#define EFONT_OTFGPOS_HH
#include <efont/otf.hh>
namespace Efont {

// Have: a list of Unicode characters
// Want: the 'simple' substitutions concerning any of those Unicode characters
// How to do: GlyphIndexSet == set of GlyphIndexes (4bits/12bits)
// Single substitutions
// Generate a list of substitutions of different types that apply?
// Use: ??que?

class OpenType

class OpenType_GPOS { public:

    EfontOTF_GPOS(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

  private:

    String _str;
    
};

}
#endif
