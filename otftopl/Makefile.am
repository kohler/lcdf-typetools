## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = otftopl
man_MANS = otftopl.1

otftopl_SOURCES = \
	dvipsencoding.cc dvipsencoding.hh \
	gsubencoding.cc gsubencoding.hh \
	md5.c md5.h \
	otftopl.cc \
	templatei.cc \
	util.cc util.hh

otftopl_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include

EXTRADIST = otftopl.1
