## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = mmpfb
man_MANS = mmpfb.1

mmpfb_SOURCES = hashmapi.cc \
	vectori.cc \
	myfont.cc myfont.hh \
	main.cc \
	t1rewrit.cc t1rewrit.hh

mmpfb_LDADD = ../libefont/libefont.a ../edlib/libedlib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/libefont

EXTRA_DIST = mmpfb.1
