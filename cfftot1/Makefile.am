## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = cfftot1
man_MANS = 

cfftot1_SOURCES = cfftot1.cc \
	templatei.cc

cfftot1_LDADD = ../libefont/libefont.a ../edlib/libedlib.a

INCLUDES = -I$(top_srcdir)/edlib -I$(top_srcdir)/libefont
