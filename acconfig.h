/* Process this file with autoheader to produce config.h.in */
#ifndef CONFIG_H
#define CONFIG_H

/* Package and version. */
#define PACKAGE "t1sicle"
#define VERSION "97"

/* Pathname separator character ('/' on Unix). */
#define PATHNAME_SEPARATOR '/'

/* Check for bad strtod and strtol. */
#undef BROKEN_STRTOARITH

/* Define to 0 if you don't want mmafm to run mmpfb when it needs to get an
   intermediate master conversion program. */
#define MMAFM_RUN_MMPFB 1

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
