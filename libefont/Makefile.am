## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = libefont.a

libefont_a_SOURCES = \
	afm.hh afm.cc \
	afmparse.hh afmparse.cc \
	afmw.hh afmw.cc \
	amfm.hh amfm.cc \
	cff.hh cff.cc \
	encoding.hh encoding.cc \
	findmet.hh findmet.cc \
	metrics.hh metrics.cc \
	otf.hh otf.cc \
	pairop.hh pairop.cc \
	psres.hh psres.cc \
	t1cs.hh t1cs.cc \
	t1csgen.hh t1csgen.cc \
	t1interp.hh t1interp.cc \
	t1item.hh t1item.cc \
	t1font.hh t1font.cc \
	t1mm.hh t1mm.cc \
	t1rw.hh t1rw.cc \
	t1unparser.hh t1unparser.cc

INCLUDES = -I$(top_srcdir)/include

srclinks:
	test $(top_srcdir) = ..
	test -d ../include || mkdir ../include
	test -d ../include/efont || mkdir ../include/efont
	I=../../libefont/include/efont S=../../libefont/libefont; \
	for i in $(libefont_a_SOURCES); do \
	test -f $$I/$$i && ln -sf ../$$I/$$i ../include/efont/$$i; \
	test -f $$S/$$i && ln -sf $$S/$$i $$i; done
