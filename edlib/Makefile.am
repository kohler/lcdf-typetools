## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign

noinst_LIBRARIES = libedlib.a

libedlib_a_SOURCES = clp.h clp.c \
	error.hh error.cc \
	filename.hh filename.cc \
	landmark.hh landmark.cc \
	permstr.hh permstr.cc \
	slurper.hh slurper.cc \
	straccum.hh straccum.cc \
	string.hh string.cc \
	strtonum.h strtonum.c

libedlib_a_LIBADD = @FIXLIBC_O@

EXTRA_DIST = hashmap.hh hashmap.cc vector.hh vector.cc fixlibc.c

AUTODEPENDENCIES =
@AUTODEPENDENCIES@

permstr.o: permstr.hh permstr.cc
straccum.o: straccum.hh straccum.cc
landmark.o: landmark.hh landmark.cc
error.o: error.hh error.cc
filename.o: filename.hh filename.cc
slurper.o: slurper.hh slurper.cc
string.o: string.hh string.cc
strtonum.o: strtonum.h strtonum.c
clp.o: clp.h clp.c
