## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = t1lint
man_MANS = 

t1lint_SOURCES = cscheck.hh cscheck.cc \
	point.hh point.cc \
	t1lint.cc \
	templatei.cc

t1lint_LDADD = ../libefont/libefont.a ../edlib/libedlib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/libefont

EXTRA_DIST = t1lint.1
