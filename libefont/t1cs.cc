#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1cs.hh"
#include "t1interp.hh"
#include <cstring>

const char * const Charstring::command_names[] = {
    "error", "hstem", "UNKNOWN_2", "vstem", "vmoveto",
    "rlineto", "hlineto", "vlineto", "rrcurveto", "closepath",
  
    "callsubr", "return", "escape", "hsbw", "endchar",
    "UNKNOWN_15", "blend", "UNKNOWN_17", "hstemhm", "hintmask",
  
    "cntrmask", "rmoveto", "hmoveto", "vstemhm", "rcurveline",
    "rlinecurve", "vvcurveto", "hhcurveto", "shortint", "callgsubr",
  
    "vhcurveto", "hvcurveto", "dotsection", "vstem3", "hstem3",
    "and", "or", "not", "seac", "sbw",
  
    "store", "abs", "add", "sub", "div",
    "load", "neg", "eq", "callothersubr", "pop",
  
    "drop", "UNKNOWN_12_19", "put", "get", "ifelse",
    "random", "mul", "UNKNOWN_12_25", "sqrt", "dup",
  
    "exch", "index", "roll", "UNKNOWN_12_31", "UNKNOWN_12_32",
    "setcurrentpoint", "hflex", "flex", "hflex1", "flex1"
};

Charstring::~Charstring()
{
}

String
Charstring::command_name(int cmd)
{
    if (cmd >= 0 && cmd <= cLastCommand)
	return command_names[cmd];
    else if (cmd < cEscapeDelta + 256)
	return String("COMMAND_12_") + String(cmd - cEscapeDelta);
    else
	return String("<INVALID>");
}


Type1Charstring::Type1Charstring(int lenIV, const String &s)
    : Charstring(), _key(-1)
{
    if (lenIV < 0)		// no charstring encryption
	_s = s;
    else if (lenIV < s.length()) {
	const unsigned char *d = reinterpret_cast<const unsigned char*>(s.data());
	_key = t1R_cs;
	for (int i = 0; i < lenIV; i++, d++)
	    _key = ((*d + _key) * t1C1 + t1C2) & 0xFFFF;
	_s = s.substring(lenIV);
    }
}

Type1Charstring::Type1Charstring(const Type1Charstring &t1cs)
    : Charstring(t1cs), _s(t1cs._s), _key(t1cs._key)
{
}

void
Type1Charstring::prepend(const Type1Charstring &t1cs)
{
    if (_key >= 0)
	decrypt();
    if (t1cs._key >= 0)
	t1cs.decrypt();
    _s = t1cs._s + _s;
}

void
Type1Charstring::decrypt() const
{
    if (_key >= 0) {
	int r = _key;
	unsigned char *d = reinterpret_cast<unsigned char*>(_s.mutable_data());
	for (int i = 0; i < _s.length(); i++, d++) {
	    unsigned char encrypted = *d;
	    *d = encrypted ^ (r >> 8);
	    r = ((encrypted + r) * t1C1 + t1C2) & 0xFFFF;
	}
	_key = -1;
    }
}

bool
Type1Charstring::run(CharstringInterp &interp) const
{
    const unsigned char *data = Type1Charstring::data();
    int left = _s.length();

    while (left > 0) {
	bool more;
	int ahead;
    
	if (*data >= 32 && *data <= 246) {		// push small number
	    more = interp.number(data[0] - 139);
	    ahead = 1;
      
	} else if (*data < 32) {			// a command
	    if (*data == cEscape) {
		if (left < 2)
		    goto runoff_error;
		more = interp.type1_command(cEscapeDelta + data[1]);
		ahead = 2;
	    } else if (*data == cShortint) { // short integer
		if (left < 3)
		    goto runoff_error;
		int val = (data[1] << 8) | data[2];
		if (val & (1L << 15))
		    val |= ~0xFFFF;
		more = interp.number(val);
		ahead = 3;
	    } else {
		more = interp.type1_command(data[0]);
		ahead = 1;
	    }
    
	} else if (*data >= 247 && *data <= 250) {	// push medium number
	    if (left < 2)
		goto runoff_error;
	    int val = + ((data[0] - 247) << 8) + 108 + data[1];
	    more = interp.number(val);
	    ahead = 2;
    
	} else if (*data >= 251 && *data <= 254) {	// push negative medium number
	    if (left < 2)
		goto runoff_error;
	    int val = - ((data[0] - 251) << 8) - 108 - data[1];
	    more = interp.number(val);
	    ahead = 2;
      
	} else {					// 255: push huge number
	    if (left < 5)
		goto runoff_error;
	    long val = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
	    if (val & (1L << 31))
		val |= ~0xFFFFFFFFL;
	    more = interp.number(val);
	    ahead = 5;
	}
    
	if (!more)
	    return interp.error() == CharstringInterp::errOK;
    
	data += ahead;
	left -= ahead;
    }
  
  runoff_error:
    interp.error(CharstringInterp::errRunoff);
    return false;
}

void
Type1Charstring::assign_substring(int pos, int len, const String &cs)
{
    if (_key >= 0)
	decrypt();
    if (pos < 0 || len < 0 || pos + len >= _s.length())
	/* do nothing */;
    else if (cs.length() <= len) {
	char *d = _s.mutable_data();
	memcpy(d + pos, cs.data(), cs.length());
	memmove(d + pos + cs.length(), d + pos + len, _s.length() - pos - len);
	_s = _s.substring(0, cs.length() - len);
    } else
	_s = _s.substring(0, pos) + cs + _s.substring(pos + len);
}


Type2Charstring::Type2Charstring(const Type2Charstring &t2cs)
    : Charstring(), _s(t2cs._s)
{
}

bool
Type2Charstring::run(CharstringInterp &interp) const
{
    const unsigned char *data = Type2Charstring::data();
    int left = _s.length();
  
    while (left > 0) {
	bool more;
	int ahead;
    
	if (*data >= 32 && *data <= 246) {		// push small number
	    more = interp.number(data[0] - 139);
	    ahead = 1;
	    
	} else if (*data < 32) {			// a command
	    if (*data == cEscape) {
		if (left < 2)
		    goto runoff_error;
		more = interp.type2_command(cEscapeDelta + data[1], 0, 0);
		ahead = 2;
	    } else if (*data == cShortint) { // short integer
		if (left < 3)
		    goto runoff_error;
		int val = (data[1] << 8) | data[2];
		if (val & (1L << 15))
		    val |= ~0xFFFF;
		more = interp.number(val);
		ahead = 3;
	    } else if (*data == cHintmask || *data == cCntrmask) {
		int left_ptr = left - 1;
		more = interp.type2_command(data[0], data + 1, &left_ptr);
		ahead = 1 + (left - 1) - left_ptr;
	    } else {
		more = interp.type2_command(data[0], 0, 0);
		ahead = 1;
	    }

	} else if (*data >= 247 && *data <= 250) {	// push medium number
	    if (left < 2)
		goto runoff_error;
	    int val = + ((data[0] - 247) << 8) + 108 + data[1];
	    more = interp.number(val);
	    ahead = 2;
    
	} else if (*data >= 251 && *data <= 254) {	// push negative medium number
	    if (left < 2)
		goto runoff_error;
	    int val = - ((data[0] - 251) << 8) - 108 - data[1];
	    more = interp.number(val);
	    ahead = 2;
      
	} else {					// 255: push huge number
	    if (left < 5)
		goto runoff_error;
	    long val = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
	    if (val & (1L << 31))
		val |= ~0xFFFFFFFFL;
	    more = interp.number(val / 65536.);
	    ahead = 5;
	}

	if (!more)
	    return interp.error() == CharstringInterp::errOK;

	data += ahead;
	left -= ahead;
    }
  
  runoff_error:
    interp.error(CharstringInterp::errRunoff);
    return false;
}


void
EfontProgram::glyph_names(Vector<PermString> &gnames) const
{
    int n = nglyphs();
    gnames.resize(n);
    for (int i = 0; i < n; i++)
	gnames[i] = glyph_name(i);
}

double
EfontProgram::global_width_x(bool) const
{
    return UNKDOUBLE;
}
