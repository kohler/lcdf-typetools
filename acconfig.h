/* Process this file with autoheader to produce config.h.in */
#ifndef CONFIG_H
#define CONFIG_H

/* Pathname separator character ('/' on Unix). */
#define PATHNAME_SEPARATOR '/'

/* Check for bad strtod. */
#undef BROKEN_STRTOD

/* Define if you have the strerror function. */
#undef HAVE_STRERROR 

/* Define to 1 since we have PermStrings. */
#define HAVE_PERMSTRING 1

/* Define if <new> exists and works. */
#undef HAVE_NEW_HDR

/* Define if you have u_intXX_t types but not uintXX_t types. */
#undef HAVE_U_INT_TYPES

/* Data directory. */
#undef SHAREDIR

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
