#ifndef OTFTOTFM_UTIL_HH
#define OTFTOTFM_UTIL_HH
#include <lcdf/string.hh>
#include <lcdf/globmatch.hh>
class ErrorHandler;

extern bool no_create;
extern bool verbose;
extern bool force;
String read_file(String filename, ErrorHandler *, bool warn = false);
String printable_filename(const String &);
String pathname_filename(const String &);
String shell_quote(const String &);
int mysystem(const char *command, ErrorHandler *);
bool parse_unicode_number(const char*, const char*, int require_prefix, uint32_t& result);

#ifdef WIN32
#define WEXITSTATUS(es) (es)
#endif

#endif
