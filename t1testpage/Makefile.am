## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign

bin_PROGRAMS = t1testpage
man_MANS = 

t1testpage_SOURCES = hashmapi.cc \
	t1testpage.cc \
	vectori.cc

t1testpage_LDADD = ../t1lib/libt1lib.a ../edlib/libedlib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/t1lib

EXTRA_DIST =

AUTODEPENDENCIES =
@AUTODEPENDENCIES@
