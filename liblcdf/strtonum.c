#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include "strtonum.h"
#ifdef __cplusplus
extern "C" {
#endif

/* A faster strtod() which only calls the real strtod() if the string has
   decimals. */

double
strtonumber(const char *f, char **endf)
{
  // in case strtol() doesn't return 0 on strange input
  if (*f == '.')
    return strtod((char *)f, endf);
  
  int v = strtol((char *)f, endf, 10);
  if (**endf == '.' && v < 0)
    return v - strtod(*endf, endf);
  else if (**endf == '.')
    return v + strtod(*endf, endf);
  else if (**endf == 'E' || **endf == 'e')
    return strtod((char *)f, endf);
  else
    return v;
}

#ifdef __cplusplus
}
#endif
