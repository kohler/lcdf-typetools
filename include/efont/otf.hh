// -*- related-file-name: "../../libefont/otf.cc" -*-
#ifndef EFONT_OTF_HH
#define EFONT_OTF_HH
#include <efont/t1cs.hh>		/* for uintXX_t definitions */
class ErrorHandler;
namespace Efont {

typedef int OpenTypeGlyph;		// 16-bit integer

class OpenTypeTag { public:
    
    enum { FIRST_VALID_TAG = 0x20202020U, LAST_VALID_TAG = 0x7E7E7E7EU };

    OpenTypeTag()			: _tag(FIRST_VALID_TAG) { }
    OpenTypeTag(uint32_t tag)		: _tag(tag) { }
    OpenTypeTag(const char *);
    OpenTypeTag(const String &);
    // default destructor

    bool ok() const			{ return _tag != 0; }
    bool check_valid() const;
    uint32_t value() const		{ return _tag; }
    String text() const;

    const uint8_t *table_entry(const uint8_t *table, int n, int entry_size) const;

    const char *script_description() const;
    const char *language_description() const;
    const char *feature_description() const;
    
  private:

    uint32_t _tag;

};

class OpenTypeFont { public:

    OpenTypeFont(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

    const String &data_string() const	{ return _str; }
    const uint8_t *data() const		{ return _str.udata(); }
    int length() const			{ return _str.length(); }

    String table(OpenTypeTag) const;

  private:

    String _str;
    int _error;

    enum { HEADER_SIZE = 12, TABLE_DIR_ENTRY_SIZE = 16 };

    int parse_header(ErrorHandler *);
    
};

class OpenTypeScriptList { public:

    OpenTypeScriptList(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _str.length() > 0; }

    int features(OpenTypeTag script, OpenTypeTag langsys, int &required_fid, Vector<int> &fids, ErrorHandler * = 0) const;
    
  private:

    enum { SCRIPTREC_SIZE = 6, LANGSYSREC_SIZE = 6 };
    
    String _str;

    int check_header(ErrorHandler *);
    int script_offset(OpenTypeTag) const;
    int langsys_offset(OpenTypeTag, OpenTypeTag, ErrorHandler * = 0) const;
    
};

class OpenTypeFeatureList { public:

    OpenTypeFeatureList(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _str.length() > 0; }

    void filter_features(Vector<int> &fids, const Vector<OpenTypeTag> &sorted_ftags) const;
    int lookups(const Vector<int> &fids, Vector<int> &results, ErrorHandler * = 0) const;
    int lookups(int required_fid, const Vector<int> &fids, const Vector<OpenTypeTag> &sorted_ftags, Vector<int> &results, ErrorHandler * = 0) const;
    
  private:

    enum { FEATUREREC_SIZE = 6 };
    
    String _str;

    int check_header(ErrorHandler *);
    int script_offset(OpenTypeTag) const;
    int langsys_offset(OpenTypeTag, OpenTypeTag, ErrorHandler * = 0) const;
    
};

class OpenTypeCoverage { public:

    OpenTypeCoverage(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _str.length() > 0; }

    int lookup(OpenTypeGlyph) const;
    int operator[](OpenTypeGlyph g) const { return lookup(g); }
    
  private:

    String _str;

    int check(ErrorHandler *);

};

class OpenTypeClassDef { public:

    OpenTypeClassDef(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _str.length() > 0; }

    int lookup(OpenTypeGlyph) const;
    int operator[](OpenTypeGlyph g) const { return lookup(g); }
    
  private:

    String _str;

    int check(ErrorHandler *);

};


inline bool
operator==(OpenTypeTag t1, uint32_t t2)
{
    return t1.value() == t2;
}

inline bool
operator!=(OpenTypeTag t1, uint32_t t2)
{
    return t1.value() != t2;
}

inline bool
operator<(OpenTypeTag t1, uint32_t t2)
{
    return t1.value() < t2;
}

inline bool
operator>(OpenTypeTag t1, uint32_t t2)
{
    return t1.value() > t2;
}

inline bool
operator<=(OpenTypeTag t1, uint32_t t2)
{
    return t1.value() <= t2;
}

inline bool
operator>=(OpenTypeTag t1, uint32_t t2)
{
    return t1.value() >= t2;
}

inline bool
operator==(OpenTypeTag t1, OpenTypeTag t2)
{
    return t1.value() == t2.value();
}

inline bool
operator!=(OpenTypeTag t1, OpenTypeTag t2)
{
    return t1.value() != t2.value();
}

inline bool
operator<(OpenTypeTag t1, OpenTypeTag t2)
{
    return t1.value() < t2.value();
}

}
#endif
