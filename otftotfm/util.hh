#ifndef OTFTOTFM_UTIL_HH
#define OTFTOTFM_UTIL_HH
#include <lcdf/string.hh>
class ErrorHandler;

String read_file(String filename, ErrorHandler *, bool warn = false);
String printable_filename(const String &);
String shell_command_output(String cmdline, const String &input, ErrorHandler *, bool strip_newlines = true);

#endif
