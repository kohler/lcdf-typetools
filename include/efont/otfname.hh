// -*- related-file-name: "../../libefont/otfname.cc" -*-
#ifndef EFONT_OTFNAME_HH
#define EFONT_OTFNAME_HH
#include <efont/otf.hh>
#include <efont/otfdata.hh>	// for ntohl()
#include <lcdf/error.hh>
namespace Efont { namespace OpenType {

class Name { public:

    Name(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

    enum NameID { N_COPYRIGHT = 0, N_FAMILY = 1, N_SUBFAMILY = 2,
		  N_UNIQUEID = 3, N_FULLNAME = 4, N_VERSION = 5,
		  N_POSTSCRIPT = 6, N_TRADEMARK = 7, N_MANUFACTURER = 8,
		  N_DESIGNER = 9, N_DESCRIPTION = 10, N_VENDOR_URL = 11,
		  N_DESIGNER_URL = 12, N_LICENSE_DESCRIPTION = 13,
		  N_LICENSE_URL = 14 };
    enum Platform { P_UNICODE = 0, P_MACINTOSH = 1, P_MICROSOFT = 3 };

    struct PlatformPred {
	inline PlatformPred(int platform = -1, int encoding = -1, int language = -1);
	inline bool operator()(int platform, int encoding, int language) const;
      private:
	int _platform, _encoding, _language;
    };
    
    struct EnglishPlatformPred {
	EnglishPlatformPred()		{ }
	inline bool operator()(int platform, int encoding, int language) const;
    };

    template <typename PlatformPredicate> String find(int nameid, PlatformPredicate) const;

  private:

    String _str;
    int _error;

    enum { HEADER_SIZE = 6, NAMEREC_SIZE = 12 };
    
    int parse_header(ErrorHandler *);
    
};

inline
Name::PlatformPred::PlatformPred(int p, int e, int l)
    : _platform(p), _encoding(e), _language(l)
{
}

inline bool
Name::PlatformPred::operator()(int p, int e, int l) const
{
    return (_platform < 0 || _platform == p)
	&& (_encoding < 0 || _encoding == e)
	&& (_language < 0 || _language == l);
}

inline bool
Name::EnglishPlatformPred::operator()(int p, int e, int l) const
{
    return (p == P_MACINTOSH && e == 0 && l == 0)
	|| (p == P_MICROSOFT && e == 1 && l == 0x409);
}


#define USHORT_AT(d)		(ntohs(*(const uint16_t *)(d)))

template <typename P>
String
Name::find(int nameid, P predicate) const
{
    if (error() < 0)
	return String();
    const uint8_t *data = _str.udata();
    int count = USHORT_AT(data + 2);
    int stringOffset = USHORT_AT(data + 4);
    data += HEADER_SIZE;
    for (int i = 0; i < count; i++, data += NAMEREC_SIZE) {
	if (nameid == USHORT_AT(data + 6)
	    && predicate(USHORT_AT(data), USHORT_AT(data + 2), USHORT_AT(data + 4))) {
	    int length = USHORT_AT(data + 8);
	    int offset = USHORT_AT(data + 10);
	    if (stringOffset + offset + length <= _str.length())
		return _str.substring(stringOffset + offset, length);
	}
    }
    return String();
}

#undef USHORT_AT

}}
#endif
