#ifndef STRING_HH
#define STRING_HH
#ifdef __GNUG__
#pragma interface
#endif
#include "permstr.hh"
#include <stdio.h>

class String {
  
  struct Memo {
    int _refcount;
    int _capacity;
    int _dirty;
    char *_real_data;
    
    Memo();
    Memo(int, int);
    ~Memo();
  };
  
  const char *_data;
  int _length;
  Memo *_memo;
  
  String(const char *, int, Memo *);
  
  void assign(const String &);
  void assign(const char *, int);
  void assign(PermString);
  void deref();
  
  static Memo *null_memo;
  static Memo *permanent_memo;
  
 public:
  
  String();
  String(const String &s);
  String(const char *cc)		{ assign(cc, -1); }
  String(const char *cc, int l)		{ assign(cc, l); }
  String(PermString p);
  explicit String(char);
  explicit String(int);
  explicit String(unsigned);
  ~String();
  
  int length() const			{ return _length; }
  const char *data() const		{ return _data; }
  char *mutable_data();
  
  operator bool()			{ return _length != 0; }
  operator bool() const			{ return _length != 0; }
  
  operator PermString()			{ return PermString(_data, _length); }
  operator PermString() const		{ return PermString(_data, _length); }
  
  const char *cc();			// pointer returned is semi-transient
  operator const char *()		{ return cc(); }
  
  char operator[](int e) const		{ return _data[e]; }
  
  friend bool operator==(const String &, const String &);
  friend bool operator==(const String &, const char *);
  friend bool operator!=(const String &, const String &);
  
  String substring(int, int) const;
  String substring(int left) const	{ return substring(left, _length); }
  
  String &operator=(const String &);
  String &operator=(const char *);
  String &operator=(PermString);
  
  void append(const char *, int);
  String &operator+=(const String &);
  String &operator+=(const char *);
  String &operator+=(PermString p);
  String &operator+=(char);
  
  // String operator+(String, const String &);
  // String operator+(String, const char *);
  // String operator+(const char *, String);
  // String operator+(String, PermString);
  // String operator+(PermString, String);
  // String operator+(PermString, const char *);
  // String operator+(const char *, PermString);
  // String operator+(PermString, PermString);
  // String operator+(String, char);
  
};


inline void
String::assign(const String &s)
{
  _data = s._data;
  _length = s._length;
  _memo = s._memo;
  _memo->_refcount++;
}

inline void
String::assign(PermString p)
{
  assert(p && "null PermString");
  _data = p.cc();
  _length = p.length();
  _memo = permanent_memo;
  _memo->_refcount++;
}

inline void
String::deref()
{
  if (--_memo->_refcount == 0) delete _memo;
}

inline
String::String()
  : _data(null_memo->_real_data), _length(0), _memo(null_memo)
{
  _memo->_refcount++;
}

inline
String::String(char c)
{
  assign(&c, 1);
}

inline
String::String(const String &s)
{
  assign(s);
}

inline
String::String(PermString p)
{
  assign(p);
}

inline
String::String(const char *data, int length, Memo *memo)
  : _data(data), _length(length), _memo(memo)
{
  _memo->_refcount++;
}

inline
String::~String()
{
  deref();
}

inline bool
operator==(const String &s1, const char *cc2)
{
  return s1 == String(cc2);
}

inline String &
String::operator=(const String &s)
{
  if (&s != this) {
    deref();
    assign(s);
  }
  return *this;
}

inline String &
String::operator=(const char *cc)
{
  deref();
  assign(cc, -1);
  return *this;
}

inline String &
String::operator=(PermString p)
{
  deref();
  assign(p);
  return *this;
}

inline String &
String::operator+=(const String &s)
{
  append(s._data, s._length);
  return *this;
}

inline String &
String::operator+=(const char *cc)
{
  append(cc, -1);
  return *this;
}

inline String &
String::operator+=(PermString p)
{
  append(p.cc(), p.length());
  return *this;
}

inline String &
String::operator+=(char c)
{
  append(&c, 1);
  return *this;
}

inline String
operator+(String s1, const String &s2)
{
  s1 += s2;
  return s1;
}

inline String
operator+(String s1, const char *cc2)
{
  s1.append(cc2, -1);
  return s1;
} 

inline String
operator+(const char *cc1, const String &s2)
{
  String s1(cc1);
  s1 += s2;
  return s1;
} 

inline String
operator+(String s1, char c2)
{
  s1.append(&c2, 1);
  return s1;
} 

inline String
operator+(String s1, PermString p2)
{
  s1.append(p2.cc(), p2.length());
  return s1;
} 

inline String
operator+(PermString p1, String s2)
{
  String s1(p1);
  return s1 + s2;
} 

inline String
operator+(PermString p1, const char *cc2)
{
  String s1(p1);
  return s1 + cc2;
} 

inline String
operator+(const char *cc1, PermString p2)
{
  String s1(cc1);
  return s1 + p2;
} 

inline String
operator+(PermString p1, PermString p2)
{
  String s1(p1);
  return s1 + p2;
} 

#endif
