## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libefont.a

libefont_a_SOURCES = \
	afm.cc \
	afmparse.cc \
	afmw.cc \
	amfm.cc \
	cff.cc \
	encoding.cc \
	findmet.cc \
	metrics.cc \
	otf.cc \
	otfcmap.cc \
	otfgsub.cc \
	otfstrings.cc \
	pairop.cc \
	psres.cc \
	t1bounds.cc \
	t1cs.cc \
	t1csgen.cc \
	t1interp.cc \
	t1item.cc \
	t1font.cc \
	t1mm.cc \
	t1rw.cc \
	t1unparser.cc

INCLUDES = -I$(top_srcdir)/include

srclinks:
	test $(top_srcdir) = ..
	test -d ../include || mkdir ../include
	test -d ../include/efont || mkdir ../include/efont
	I=../../libefont/include/efont S=../../libefont/libefont; \
	for i in $(libefont_a_SOURCES); do \
	j=`echo $$i | sed 's/.cc$$//'`; \
	test -f $$I/$$j.hh && ln -sf ../$$I/$$j.hh ../include/efont/$$j.hh; \
	test -f $$S/$$i && ln -sf $$S/$$i $$i; done
