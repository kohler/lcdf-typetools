#ifndef OTFTOTFM_UTIL_HH
#define OTFTOTFM_UTIL_HH
#include <lcdf/string.hh>
class ErrorHandler;

extern bool nocreate;
extern bool verbose;
String read_file(String filename, ErrorHandler *, bool warn = false);
String printable_filename(const String &);
int mysystem(const char *command, ErrorHandler *);

#endif
