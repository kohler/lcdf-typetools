// -*- related-file-name: "../include/efont/t1csgen.hh" -*-
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <efont/t1csgen.hh>
#include <cmath>
namespace Efont {

static const char * const command_desc[] = {
    0, 0, 0, 0, "y",
    "xy", "x", "y", "xyxyxy", 0,

    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,

    0, "xy", "x", 0, 0,
    0, 0, 0, 0, 0,

    "yxyx", "xxyy", 0, 0, 0,
    0, 0, 0, 0, 0,

    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
  
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
  
    0, 0, 0, 0, 0,
    "XY", 0, 0, 0, 0
};

Type1CharstringGen::Type1CharstringGen(int precision)
{
    if (precision >= 1 && precision <= 107)
	_precision = precision;
    else
	_precision = 5;
    _f_precision = _precision;
}

void
Type1CharstringGen::clear()
{
    _ncs.clear();
    _true_x = _true_y = _false_x = _false_y = 0;
}

void
Type1CharstringGen::gen_number(double float_val, int kind)
{
    switch (kind) {
      case 'x':
	_true_x += float_val;
	float_val = _true_x - _false_x;
	break;
      case 'y':
	_true_y += float_val;
	float_val = _true_y - _false_y;
	break;
      case 'X':
	_true_x = float_val;
	break;
      case 'Y':
	_true_y = float_val;
	break;
    }
    
    int big_val = (int)floor(float_val * _f_precision + 0.5);
    int val = big_val / _precision;
    int frac = big_val % _precision;
    if (frac != 0)
	val = big_val;

    if (val >= -107 && val <= 107)
	_ncs.append((char)(val + 139));

    else if (val >= -1131 && val <= 1131) {
	int base = val < 0 ? 251 : 247;
	if (val < 0) val = -val;
	val -= 108;
	int w = val % 256;
	val = (val - w) / 256;
	_ncs.append((char)(val + base));
	_ncs.append((char)w);

    } else {
	_ncs.append('\377');
	long l = val;
	_ncs.append((char)((l >> 24) & 0xFF));
	_ncs.append((char)((l >> 16) & 0xFF));
	_ncs.append((char)((l >> 8) & 0xFF));
	_ncs.append((char)((l >> 0) & 0xFF));
    }

    if (frac != 0) {
	_ncs.append((char)(_precision + 139));
	_ncs.append((char)Charstring::cEscape);
	_ncs.append((char)(Charstring::cDiv - Charstring::cEscapeDelta));
    }

    float_val = (double)big_val / _precision;
    switch (kind) {
      case 'x':
	_false_x += float_val;
	break;
      case 'y':
	_false_y += float_val;
	break;
      case 'X':
	_false_x = float_val;
	break;
      case 'Y':
	_false_y = float_val;
	break;
    }
}


void
Type1CharstringGen::gen_command(int command)
{
    if (command >= Charstring::cEscapeDelta) {
	_ncs.append((char)Charstring::cEscape);
	_ncs.append((char)(command - Charstring::cEscapeDelta));
    } else
	_ncs.append((char)command);
}

void
Type1CharstringGen::gen_stack(CharstringInterp &interp, int for_cmd)
{
    const char *str = ((unsigned)for_cmd <= Charstring::cLastCommand ? command_desc[for_cmd] : (const char *)0);
    int i;
    for (i = 0; str && *str && i < interp.size(); i++, str++)
	gen_number(interp.at(i), *str);
    for (; i < interp.size(); i++)
	gen_number(interp.at(i));
    interp.clear();
}

void
Type1CharstringGen::append_charstring(const String &s)
{
    _ncs << s;
}

Type1Charstring *
Type1CharstringGen::output()
{
    return new Type1Charstring(_ncs.take_string());
}

void
Type1CharstringGen::output(Type1Charstring &cs)
{
    cs.assign(_ncs.take_string());
}

String
Type1CharstringGen::callsubr_string(int subr)
{
    Type1CharstringGen csg;
    csg.gen_number(subr);
    csg.gen_command(Charstring::cCallsubr);
    return csg._ncs.take_string();
}

}
