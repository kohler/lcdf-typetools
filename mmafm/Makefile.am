## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign

bin_PROGRAMS = mmafm
man_MANS = mmafm.1

mmafm_SOURCES = vectori.cc \
	hashmapi.cc \
	main.cc

mmafm_LDADD = ../metricslib/libmetrics.a ../t1lib/libt1lib.a ../edlib/libedlib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/t1lib -I$(top_srcdir)/metricslib

EXTRA_DIST = mmafm.1

AUTODEPENDENCIES =
@AUTODEPENDENCIES@
