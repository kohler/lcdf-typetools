## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = mmpfb
man_MANS = mmpfb.1

mmpfb_SOURCES = templatei.cc \
	myfont.cc myfont.hh \
	main.cc \
	t1rewrit.cc t1rewrit.hh

mmpfb_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = mmpfb.1
