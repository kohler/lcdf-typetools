## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = otftoafm
man_MANS = 

otftoafm_SOURCES = \
	dvipsencoding.cc dvipsencoding.hh \
	gsubencoding.cc gsubencoding.hh \
	md5.c md5.h \
	otftoafm.cc \
	templatei.cc \
	util.cc util.hh

otftoafm_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include
