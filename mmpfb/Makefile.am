## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign

bin_PROGRAMS = mmpfb
man_MANS = mmpfb.1

mmpfb_SOURCES = hashmapi.cc \
	vectori.cc \
	myfont.cc myfont.hh \
	main.cc \
	t1rewrit.cc t1rewrit.hh

mmpfb_LDADD = ../edlib/libedlib.a ../t1lib/libt1lib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/t1lib

EXTRA_DIST = mmpfb.1

t1rewrit.o: t1rewrit.cc t1rewrit.hh $(top_srcdir)/t1lib/t1interp.hh
myfont.o: myfont.cc myfont.hh $(top_srcdir)/t1lib/t1font.hh
