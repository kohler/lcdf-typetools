#ifndef OTFTOTFM_AUTOMATIC_HH
#define OTFTOTFM_AUTOMATIC_HH
#include <lcdf/string.hh>
class ErrorHandler;

enum { O_ENCODING = 0, O_TFM, O_PL, O_VF, O_VPL, O_TYPE1, O_MAP, NUMODIR };

extern bool automatic;
String getodir(int o, ErrorHandler *);
bool setodir(int o, const String &);
bool set_vendor(const String &);
const char *odirname(int o);
void update_odir(int o, String file, ErrorHandler *);
void set_typeface(const String &);
String installed_type1(const String &opentype_filename, ErrorHandler *);
int update_autofont_map(const String &fontname, String mapline, ErrorHandler *);
String locate_encoding(String encfile, ErrorHandler *, bool literal = false);

#endif
