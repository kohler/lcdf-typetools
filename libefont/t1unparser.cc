#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "t1unparser.hh"

Type1Unparser::Type1Unparser()
    : CharstringInterp(0, 0),
      _one_command_per_line(false), _start_of_line(true)
{
}

Type1Unparser::Type1Unparser(const Type1Unparser &o)
    : CharstringInterp(o),
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
Type1Unparser::type1_command(int cmd)
{
    if (_start_of_line) {
	_sa << _indent;
	_start_of_line = false;
    } else
	_sa << ' ';
    if (cmd >= 0 && cmd <= CS::cLastCommand)
	_sa << CS::command_names[cmd];
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
