#ifndef T1RW_HH
#define T1RW_HH
#ifdef __GNUG__
#pragma interface
#endif
#include <stdio.h>
#include "permstr.hh"
#include "straccum.hh"

class Type1Reader {
  
  static const int DataSize = 1024;
  
  unsigned char *_data;
  int _len;
  int _pos;
  
  PermString _charstring_definer;
  int _charstring_start;
  int _charstring_len;
  
  int _ungot;
  
  bool _eexec;
  bool _binary_eexec;
  int _r;
  
  Type1Reader(const Type1Reader &);
  Type1Reader &operator=(const Type1Reader &);
  
  int more_data();
  
  int eexec(int);
  int ascii_eexec_get();
  int get_base();
  int get();
  
  void start_eexec();
  
  bool test_charstring(StringAccum &);
  
  static unsigned char xvalue_store[];
  static unsigned char *xvalue;
  static void static_initialize();
  
 public:
  
  Type1Reader();
  virtual ~Type1Reader();
  
  virtual int more_data(unsigned char *, int) = 0;
  virtual bool preserve_whitespace() const { return false; }
  
  virtual void charstring_section(bool issubr);
  virtual void switch_eexec(bool);
  virtual void set_charstring_definer(PermString);
  
  bool next_line(StringAccum &);
  bool was_charstring() const		{ return _charstring_len > 0; }
  int charstring_start() const		{ return _charstring_start; }
  int charstring_length() const		{ return _charstring_len; }
  
};


class Type1PfaReader: public Type1Reader {
  
  FILE *_f;
  
 public:
  
  Type1PfaReader(FILE *);
  
  int more_data(unsigned char *, int);
  
};


class Type1PfbReader: public Type1Reader {
  
  FILE *_f;
  bool _binary;
  int _left;
  
 public:
  
  Type1PfbReader(FILE *);
  
  int more_data(unsigned char *, int);
  bool preserve_whitespace() const;
  
};


/*****
 * Writers
 **/

class Type1Writer {
  
  static const int BufSize = 1024;
  
  unsigned char *_buf;
  int _pos;
  
  bool _eexec;
  int _eexec_start;
  int _eexec_end;
  int _r;
  
  void local_flush();
  unsigned char eexec(int);
  
  Type1Writer(const Type1Writer &);
  Type1Writer &operator=(const Type1Writer &);
  
 public:
  
  Type1Writer();
  virtual ~Type1Writer();
  
  bool eexecing() const				{ return _eexec; }
  
  void print(int);
  void print(const char *, int);
  
  Type1Writer &operator<<(char);
  Type1Writer &operator<<(unsigned char);
  Type1Writer &operator<<(const char *);
  Type1Writer &operator<<(PermString);
  Type1Writer &operator<<(int);
  Type1Writer &operator<<(double);
  
  virtual void flush();
  virtual void switch_eexec(bool);
  virtual void print0(const unsigned char *, int) = 0;
  
};


class Type1PfaWriter: public Type1Writer {
  
  FILE *_f;
  int _hex_line;
  
 public:
  
  Type1PfaWriter(FILE *);
  ~Type1PfaWriter();
  
  void switch_eexec(bool);
  void print0(const unsigned char *, int);
  
};


class Type1PfbWriter: public Type1Writer {
  
  StringAccum _save;
  FILE *_f;
  bool _binary;
  
 public:
  
  Type1PfbWriter(FILE *);
  ~Type1PfbWriter();
  
  void flush();
  void switch_eexec(bool);
  void print0(const unsigned char *, int);
  
};


inline void
Type1Writer::print(int c)
{
  if (_pos >= BufSize) local_flush();
  _buf[_pos++] = c;
}

inline Type1Writer &
Type1Writer::operator<<(const char *cc)
{
  print(cc,strlen(cc));
  return *this;
}

inline Type1Writer &
Type1Writer::operator<<(PermString p)
{
  print(p, p.length());
  return *this;
}

inline Type1Writer &
Type1Writer::operator<<(char c)
{
  print((unsigned char)c);
  return *this;
}

inline Type1Writer &
Type1Writer::operator<<(unsigned char c)
{
  print(c);
  return *this;
}

#endif
