#ifndef T1UNPARSER_HH
#define T1UNPARSER_HH
#include "t1interp.hh"
#include "straccum.hh"
#include "string.hh"

class Type1Unparser : public CharstringInterp {

  String _indent;
  bool _one_command_per_line;
  bool _start_of_line;
  StringAccum _sa;

 public:

  Type1Unparser();
  Type1Unparser(const Type1Unparser &);

  const String &indent() const		{ return _indent; }
  void set_indent(const String &s)	{ _indent = s; }
  void set_one_command_per_line(bool b)	{ _one_command_per_line = b; }

  void clear();

  bool number(double);
  bool type1_command(int);

  String value();

  static String unparse(const Type1Charstring *);
  
};

#endif
