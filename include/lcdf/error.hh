#ifndef ERROR_HH
#define ERROR_HH
#ifdef __GNUG__
#pragma interface
#endif
class Landmark;

void fatal_error(const Landmark &, const char *, ...);
bool error(const Landmark &, const char *, ...);
bool warning(const Landmark &, const char *, ...);
void set_error_context(const char *err = 0);

extern int num_errors;
extern int num_warnings;

#endif
