#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1cs.hh"
#include "t1interp.hh"

Type1Charstring::Type1Charstring(int lenIV, unsigned char *d, int l)
{
  if (lenIV < 0) {
    // lenIV < 0 means there is no charstring encryption.
    _data = new unsigned char[l];
    _len = l;
    _key = -1;
  } else {
    _data = new unsigned char[l - lenIV];
    _len = l - lenIV;
    _key = t1R_cs;
    for (int i = 0; i < lenIV; i++, d++)
      _key = ((*d + _key) * t1C1 + t1C2) & 0xFFFF;
  }
  memcpy(_data, d, _len);
}

Type1Charstring::Type1Charstring(const Type1Charstring &t1cs)
  : _data(new unsigned char[t1cs._len]), _len(t1cs._len), _key(t1cs._key)
{
  memcpy(_data, t1cs._data, _len);
}

void
Type1Charstring::decrypt() const
{
  if (_key < 0) return;
  int r = _key;
  unsigned char *d = _data;
  for (int i = 0; i < _len; i++, d++) {
    unsigned char encrypted = *d;
    *d = encrypted ^ (r >> 8);
    r = ((encrypted + r) * t1C1 + t1C2) & 0xFFFF;
  }
  _key = -1;
}

bool
Type1Charstring::run(Type1Interp &t1interp) const
{
  unsigned char *data = Type1Charstring::data();
  int left = _len;
  
  while (left > 0) {
    bool more;
    int ahead;
    
    if (*data >= 32 && *data <= 246) {		// push small number
      more = t1interp.number(data[0] - 139);
      ahead = 1;
      
    } else if (*data == Type1Interp::cShortint) {	// push short integer
      if (left < 3) goto runoff_error;
      int val = (data[1] << 8) | data[2];
      if (val & (1L << 15))
	val |= ~0xFFFF;
      more = t1interp.number(val);
      ahead = 3;
      
    } else if (*data < 32) {			// a command
      if (*data == Type1Interp::cEscape) {
	if (left < 2) goto runoff_error;
	more = t1interp.command(Type1Interp::cEscapeDelta + data[1]);
	ahead = 2;
      } else {
	more = t1interp.command(data[0]);
	ahead = 1;
      }
    
    } else if (*data >= 247 && *data <= 250) {	// push medium number
      if (left < 2) goto runoff_error;
      int val = + ((data[0] - 247) << 8) + 108 + data[1];
      more = t1interp.number(val);
      ahead = 2;
    
    } else if (*data >= 251 && *data <= 254) {	// push negative medium number
      if (left < 2) goto runoff_error;
      int val = - ((data[0] - 251) << 8) - 108 - data[1];
      more = t1interp.number(val);
      ahead = 2;
      
    } else {					// 255: push huge number
      if (left < 5) goto runoff_error;
      long val = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
      if (val & (1L << 31))
	val |= ~0xFFFFFFFFL;
      more = t1interp.number(val);
      ahead = 5;
    }
    
    if (!more)
      return t1interp.errno() == Type1Interp::errOK;
    
    data += ahead;
    left -= ahead;
  }
  
 runoff_error:
  t1interp.error(Type1Interp::errRunoff);
  return false;
}
