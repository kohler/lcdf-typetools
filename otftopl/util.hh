#ifndef OTFTOAFM_UTIL_HH
#define OTFTOAFM_UTIL_HH
#include <lcdf/string.hh>
class ErrorHandler;

String read_file(String filename, ErrorHandler *, bool warn = false);
String printable_filename(const String &);

#endif
