/* Process this file with autoheader to produce config.h.in */
#ifndef CONFIG_H
#define CONFIG_H

/* Pathname separator character ('/' on Unix). */
#define PATHNAME_SEPARATOR '/'

/* Check for bad strtod. */
#undef BROKEN_STRTOD

/* Define if you have the strerror function. */
#undef HAVE_STRERROR 

/* Define to 1 since we have PermStrings and Strings. */
#define HAVE_PERMSTRING 1
#define HAVE_STRING 1

@TOP@
@BOTTOM@

#ifdef __cplusplus
extern "C" {
#endif

/* Prototype strerror if we don't have it. */
#ifndef HAVE_STRERROR
char *strerror(int errno);
#endif

/* Prototype good_strtod and good_strtol if we need them. */
#ifdef BROKEN_STRTOARITH
long good_strtol(const char *nptr, char **endptr, int base);
double good_strtod(const char *nptr, char **endptr);
#endif

#ifdef __cplusplus
}
/* Get rid of a possible inline macro under C++. */
# undef inline
#endif
#endif
