#ifndef EFONT_OTF_HH
#define EFONT_OTF_HH
#include "t1cs.hh"			/* for uintXX_t definitions */
class ErrorHandler;
namespace Efont {

class EfontOTF { public:

    EfontOTF(const String &, ErrorHandler * = 0);
    // default destructor

    bool ok() const			{ return _error >= 0; }
    int error() const			{ return _error; }

    const String &data_string() const	{ return _str; }
    const uint8_t *data() const		{ return _str.udata(); }
    int length() const			{ return _str.length(); }

    String table(const char *) const;

  private:

    String _str;
    int _error;

    enum { HEADER_SIZE = 12, TABLE_DIR_ENTRY_SIZE = 16 };

    int parse_header(ErrorHandler *);
    
};

}
#endif
