## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libedlib.a

libedlib_a_SOURCES = clp.h clp.c \
	error.hh error.cc \
	filename.hh filename.cc \
	landmark.hh landmark.cc \
	permstr.hh permstr.cc \
	slurper.hh slurper.cc \
	straccum.hh straccum.cc \
	string.hh string.cc \
	strtonum.h strtonum.c \
	vectorv.cc

libedlib_a_LIBADD = @FIXLIBC_O@

EXTRA_DIST = hashmap.hh hashmap.cc vector.hh vector.cc fixlibc.c
