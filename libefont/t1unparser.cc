#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1unparser.hh"

static const char *command_names[] = {
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
  "setcurrentpoint"
};

Type1Unparser::Type1Unparser()
  : Type1Interp(0, 0),
    _one_command_per_line(false), _start_of_line(true)
{
}

Type1Unparser::Type1Unparser(const Type1Unparser &o)
  : Type1Interp(o),
    _one_command_per_line(o._one_command_per_line),
    _start_of_line(o._start_of_line)
{
}

void
Type1Unparser::clear()
{
  _sa.clear();
  _start_of_line = true;
}

bool
Type1Unparser::number(double n)
{
  if (_start_of_line) {
    _sa << _indent;
    _start_of_line = false;
  } else
    _sa << ' ';
  _sa << n;
  return true;
}

bool
Type1Unparser::command(int cmd)
{
  if (_start_of_line) {
    _sa << _indent;
    _start_of_line = false;
  } else
    _sa << ' ';
  if (cmd >= 0 && cmd <= cLastCommand)
    _sa << command_names[cmd];
  else
    _sa << "UNKNOWN_12_" << (cmd - 32);
  if (_one_command_per_line) {
    _sa << '\n';
    _start_of_line = true;
  }
  return true;
}

String
Type1Unparser::value()
{
  _start_of_line = true;
  return _sa.take_string();
}

String
Type1Unparser::unparse(const Type1Charstring *cs)
{
  if (cs) {
    Type1Unparser u;
    cs->run(u);
    return u.value();
  } else
    return "(null)";
}

String
Type1Unparser::unparse_command(int cmd)
{
  if (cmd >= 0 && cmd <= cLastCommand)
    return command_names[cmd];
  else
    return String("UNKNOWN_12_") + String(cmd - 32);
}
