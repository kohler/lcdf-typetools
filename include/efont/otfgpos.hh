// -*- related-file-name: "../../libefont/otfgpos.cc" -*-
#ifndef EFONT_OTFGPOS_HH
#define EFONT_OTFGPOS_HH
#include <efont/otf.hh>
namespace Efont {

class OpenTypeGlyphTable { public:

    OpenTypeGlyphTable(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

    int scripts(Vector<OpenTypeTag> &) const;
    int languages(OpenTypeTag script, Vector<OpenTypeTag> &, ErrorHandler * = 0) const;
    int features(Vector<OpenTypeTag> &) const;
    int features(OpenTypeTag script, OpenTypeTag language, Vector<OpenTypeTag> &, ErrorHandler * = 0) const;
    int lookups(OpenTypeTag script, OpenTypeTag language, const Vector<OpenTypeTag> &features, Vector<const uint8_t *> &, ErrorHandler * = 0) const;
    
    const char *table_name() const	{ return "GlyphTable"; }

  private:

    String _str;
    int _error;

    enum {
	HEADER_SIZE = 10,
	SCRIPTLIST_HEADER_SIZE = 2, SCRIPT_RECORD_SIZE = 6,
	SCRIPT_HEADER_SIZE = 4, LANGSYS_RECORD_SIZE = 6,
	LANGSYS_HEADER_SIZE = 6, FEATINDEX_RECORD_SIZE = 2,
	FEATURELIST_HEADER_SIZE = 2, FEATURE_RECORD_SIZE = 6,
	FEATURE_HEADER_SIZE = 4, LOOKUPINDEX_RECORD_SIZE = 2,
	LOOKUPLIST_HEADER_SIZE = 2, LOOKUP_RECORD_SIZE = 2
    };
    
    int parse_header(ErrorHandler *);
    const uint8_t *script_table(OpenTypeTag script, ErrorHandler *) const;
    const uint8_t *langsys_table(OpenTypeTag script, OpenTypeTag language, bool allow_default, ErrorHandler *) const;
    int lookups(int findex, Vector<const uint8_t *> &, ErrorHandler *) const;
    
};


class EfontOTF_GPOS { public:

    EfontOTF_GPOS(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

  private:

    String _str;
    int _error;
    int _ntables;
    mutable Vector<int> _table_error;

    enum { HEADER_SIZE = 4, ENCODING_SIZE = 8 };
    enum Format { F_BYTE = 0, F_HIBYTE = 2, F_SEGMENTED = 4, F_TRIMMED = 6,
		  F_HIBYTE32 = 8, F_TRIMMED32 = 10, F_SEGMENTED32 = 12 };
    
    int parse_header(ErrorHandler *);
    int first_unicode_table() const;
    int first_table(int platform, int encoding) const;
    int check_table(int t, ErrorHandler * = 0) const;
    uint32_t map_table(int t, uint32_t uni, ErrorHandler * = 0) const;
    uint32_t dump_table(int t, Vector<uint32_t> &cs, Vector<uint32_t> &gs, ErrorHandler * = 0) const;
    
};

}
#endif
