## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = t1testpage
man_MANS = 

t1testpage_SOURCES = t1testpage.cc \
	templatei.cc

t1testpage_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include
