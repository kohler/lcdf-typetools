#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "string.hh"
#include <string.h>
#include <stdio.h>

String::Memo *String::null_memo;
String::Memo *String::permanent_memo;
static String::Initializer initializer;


String::Memo::Memo()
  : _refcount(1), _capacity(0), _dirty(0), _real_data("")
{
}

String::Memo::Memo(int dirty, int capacity)
  : _refcount(1), _capacity(capacity), _dirty(dirty),
    _real_data(new char[capacity])
{
  assert(_capacity >= _dirty);
}

String::Memo::~Memo()
{
  assert(_capacity >= _dirty);
  delete[] _real_data;
}


String::String(int i)
{
  char buf[128];
  sprintf(buf, "%d", i);
  assign(buf, -1);
}

String::String(unsigned u)
{
  char buf[128];
  sprintf(buf, "%u", u);
  assign(buf, -1);
}

void
String::assign(const char *cc, int cclen)
{
  if (!cc)
    cclen = 0;
  else if (cclen < 0)
    cclen = strlen(cc);
  
  if (cclen == 0) {
    _memo = null_memo;
    _memo->_refcount++;
    
  } else {
    // Make `capacity' a multiple of 16 characters at least as big as `cclen'.
    int capacity = (cclen + 16) & ~15;
    _memo = new Memo(cclen, capacity);
    memcpy(_memo->_real_data, cc, cclen);
  }
  
  _data = _memo->_real_data;
  _length = cclen;
}

void
String::append(const char *cc, int cclen)
{
  if (!cc)
    cclen = 0;
  else if (cclen < 0)
    cclen = strlen(cc);
  
  if (cclen == 0)
    return;
  
  // If we can, append into unused space. First, we check that there's enough
  // unused space for `cclen' characters to fit; then, we check that the
  // unused space immediately follows the data in `*this'.
  if (_memo->_capacity > _memo->_dirty + cclen) {
    char *real_dirty = _memo->_real_data + _memo->_dirty;
    if (real_dirty == _data + _length) {
      memcpy(real_dirty, cc, cclen);
      _length += cclen;
      _memo->_dirty += cclen;
      assert(_memo->_dirty < _memo->_capacity);
      return;
    }
  }
  
  // Now we have to make new space. Make sure the new capacity is a
  // multiple of 16 characters and that it is at least 16.
  int new_capacity = (_length + 16) & ~15;
  while (new_capacity < _length + cclen)
    new_capacity *= 2;
  Memo *new_memo = new Memo(_length + cclen, new_capacity);
  
  char *new_data = new_memo->_real_data;
  memcpy(new_data, _data, _length);
  memcpy(new_data + _length, cc, cclen);
  
  deref();
  _data = new_data;
  _length += cclen;
  _memo = new_memo;
}

char *
String::mutable_data()
{
  // If _memo has a capacity (it's not one of the special strings) and it's
  // uniquely referenced, return _data right away.
  if (_memo->_capacity && _memo->_refcount == 1)
    return const_cast<char *>(_data);
  
  // Otherwise, make a copy of it. Rely on fact that deref() doesn't change
  // _data or _length.
  assert(_memo->_refcount > 1);
  deref();
  assign(_data, _length);
  return const_cast<char *>(_data);
}

const char *
String::cc()
{
  // If _memo has no capacity, then this is one of the special strings
  // (null or PermString), and we can return _data immediately.
  if (!_memo->_capacity)
    return _data;
  
  // If _memo->_capacity > 0, this invariant must hold (there's more real data
  // in _memo than in our substring).
  assert(_memo->_real_data + _memo->_dirty >= _data + _length);
  
  // Once we return a cc() from a given String, we don't want to append to
  // it, since the terminating \0 would get overwritten.
  if (_memo->_real_data + _memo->_dirty == _data + _length) {
    if (_memo->_dirty < _memo->_capacity)
      goto add_final_nul;
    
  } else {
    // OK -- someone has added characters past the end of our substring of
    // _memo. Still OK to return _data immediately if _data[_length] == '\0'.
    if (_data[_length] == '\0')
      return _data;
  }
  
  // Unfortunately, we've got a non-null-terminated substring, so we need to
  // make a copy of our portion.
  {
    String s(_data, _length);
    deref();
    assign(s);
  }
  
 add_final_nul:
  char *real_data = const_cast<char *>(_data);
  real_data[_length] = '\0';
  _memo->_dirty++;		// include '\0' in used portion of _memo
  return _data;
}

String
String::substring(int left, int len) const
{
  if (left < 0)
    left += _length;
  if (len < 0)
    len = _length - left;
  if (left + len > _length)
    len = _length - left;
  if (left < 0 || len <= 0)
    return String();
  else
    return String(_data + left, len, _memo);
}

bool
operator==(const String &s1, const String &s2)
{
  if (s1._length != s2._length) return false;
  if (s1._data == s2._data) return true;
  return memcmp(s1._data, s2._data, s1._length) == 0;
}

bool
operator!=(const String &s1, const String &s2)
{
  if (s1._length != s2._length) return true;
  if (s1._data == s2._data) return false;
  return memcmp(s1._data, s2._data, s1._length) != 0;
}


String::Initializer::Initializer()
{
  // do-nothing function called simply to initialize static globals
  if (!String::null_memo) {
    String::null_memo = new String::Memo();
    String::permanent_memo = new String::Memo();
  }
}
