## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = otftotfm
man_MANS = otftotfm.1

otftotfm_SOURCES = \
	automatic.cc automatic.hh \
	dvipsencoding.cc dvipsencoding.hh \
	gsubencoding.cc gsubencoding.hh \
	md5.c md5.h \
	otftotfm.cc \
	util.cc util.hh
EXTRA_otftotfm_SOURCES = kpseinterface.c kpseinterface.h

otftotfm_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a @KPSEINTERFACE_O@ @LIBKPATHSEA@
otftotfm_DEPENDENCIES = @KPSEINTERFACE_O@

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = otftotfm.1
