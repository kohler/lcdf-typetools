#ifndef OTFTOTFM_UTIL_HH
#define OTFTOTFM_UTIL_HH
#include <lcdf/string.hh>
class ErrorHandler;

extern bool no_create;
extern bool verbose;
extern bool force;
String read_file(String filename, ErrorHandler *, bool warn = false);
String printable_filename(const String &);
String pathname_filename(const String &);
int mysystem(const char *command, ErrorHandler *);
bool glob_match(const String &, const String &);

#endif
