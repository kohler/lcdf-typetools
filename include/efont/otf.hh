// -*- related-file-name: "../../libefont/otf.cc" -*-
#ifndef EFONT_OTF_HH
#define EFONT_OTF_HH
#include <efont/t1cs.hh>		/* for uintXX_t definitions */
class ErrorHandler;
namespace Efont {

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
