## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = cfftot1
man_MANS = 

cfftot1_SOURCES = cfftot1.cc \
	maket1font.cc maket1font.hh \
	point.cc point.hh \
	transform.cc transform.hh \
	templatei.cc

cfftot1_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include
