## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign

bin_PROGRAMS = t1lint
man_MANS = 

t1lint_SOURCES = cscheck.hh cscheck.cc \
	hashmapi.cc \
	main.cc \
	point.hh point.cc \
	vectori.cc

t1lint_LDADD = ../t1lib/libt1lib.a ../edlib/libedlib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/t1lib

EXTRA_DIST = t1lint.1
