## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = otftoafm
man_MANS = 

otftoafm_SOURCES = otftoafm.cc \
	templatei.cc

otftoafm_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include
