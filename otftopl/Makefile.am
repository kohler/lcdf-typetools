## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = otftotfm
man_MANS = otftopl.1

otftotfm_SOURCES = \
	automatic.cc automatic.hh \
	dvipsencoding.cc dvipsencoding.hh \
	gsubencoding.cc gsubencoding.hh \
	md5.c md5.h \
	otftopl.cc \
	util.cc util.hh

otftotfm_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = otftopl.1
