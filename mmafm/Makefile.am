## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = mmafm
man_MANS = mmafm.1

mmafm_SOURCES = templatei.cc \
	main.cc

mmafm_LDADD = ../libefont/libefont.a ../liblcdf/liblcdf.a

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = mmafm.1
