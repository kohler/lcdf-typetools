/* Process this file with autoheader to produce config.h.in */
#ifndef CONFIG_H
#define CONFIG_H

/* Pathname separator character ('/' on Unix). */
#define PATHNAME_SEPARATOR '/'

/* Define to 1 since we have PermStrings. */
#define HAVE_PERMSTRING 1

@TOP@
@BOTTOM@

#ifdef __cplusplus
extern "C" {
#endif

/* Prototype strerror if we don't have it. */
#ifndef HAVE_STRERROR
char *strerror(int errno);
#endif

/* Prototype good_strtod if we need it. */
#ifdef BROKEN_STRTOD
double good_strtod(const char *nptr, char **endptr);
#endif

#ifdef __cplusplus
}
/* Get rid of a possible inline macro under C++. */
# undef inline
#endif
#endif
