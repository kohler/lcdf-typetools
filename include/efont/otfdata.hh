// -*- related-file-name: "../../libefont/otfdata.cc" -*-
#ifndef EFONT_OTFDATA_HH
#define EFONT_OTFDATA_HH
#include <lcdf/string.hh>
#include <efont/t1cs.hh>		/* for integer types */
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>		/* for ntohl() */
#elif defined(HAVE_SYS_PARAM_H)
# include <sys/param.h>
#else
# error "configury disaster! Report this error to kohler@icir.org"
#endif
class ErrorHandler;
namespace Efont { namespace OpenType {

class Error { public:
    String description;
    Error()				: description(String::stable_string("unspecified error")) { }
    Error(const String &d)		: description(d) { }
};

class Bounds : public Error { public:
    Bounds()				: Error(String::stable_string("bounds error")) { }
};

class Format : public Error { public:
    Format(const String &name)		: Error(name + " format error") { }
};

class Data { public:

    Data()				{ }
    Data(const String &str)		: _str(str) { _str.align(2); }
    // default copy constructor
    // default destructor

    operator const String &() const	{ return _str; }

    int length() const			{ return _str.length(); }

    uint8_t operator[](unsigned offset) const throw (Bounds);
    uint16_t u16(unsigned offset) const throw (Bounds);
    int16_t s16(unsigned offset) const throw (Bounds);
    uint32_t u32(unsigned offset) const throw (Bounds);    
    int32_t s32(unsigned offset) const throw (Bounds);

    Data subtable(unsigned offset) const throw (Bounds);
    Data offset_subtable(unsigned offset_offset) const throw (Bounds);

  private:

    String _str;
    
};

inline uint8_t
Data::operator[](unsigned offset) const throw (Bounds)
{
    if (offset >= static_cast<unsigned>(_str.length()))
	throw Bounds();
    else
	return _str[offset];
}

inline uint16_t
Data::u16(unsigned offset) const throw (Bounds)
{
    if (offset >= static_cast<unsigned>(_str.length() - 1))
	throw Bounds();
    else
	return ntohs(*reinterpret_cast<const uint16_t *>(_str.data() + offset));
}

inline int16_t
Data::s16(unsigned offset) const throw (Bounds)
{
    if (offset >= static_cast<unsigned>(_str.length() - 1))
	throw Bounds();
    else
	return ntohs(*reinterpret_cast<const int16_t *>(_str.data() + offset));
}

inline uint32_t
Data::u32(unsigned offset) const throw (Bounds)
{
    if (offset >= static_cast<unsigned>(_str.length() - 3))
	throw Bounds();
    else
	return (ntohs(*reinterpret_cast<const uint16_t *>(_str.data() + offset)) << 16)
	    | ntohs(*reinterpret_cast<const uint16_t *>(_str.data() + offset + 2));
}

inline int32_t
Data::s32(unsigned offset) const throw (Bounds)
{
    if (offset >= static_cast<unsigned>(_str.length() - 3))
	throw Bounds();
    else
	return (ntohs(*reinterpret_cast<const uint16_t *>(_str.data() + offset)) << 16)
	    | ntohs(*reinterpret_cast<const uint16_t *>(_str.data() + offset + 2));
}

}}
#endif
