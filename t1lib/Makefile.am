## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign

noinst_LIBRARIES = libt1lib.a

libt1lib_a_SOURCES = t1cs.hh t1cs.cc \
	t1interp.hh t1interp.cc \
	t1item.hh t1item.cc \
	t1font.hh t1font.cc \
	t1mm.hh t1mm.cc \
	t1rw.hh t1rw.cc \
	t1unparser.hh t1unparser.cc \
	psres.hh psres.cc

INCLUDES = -I$(top_srcdir)/edlib

AUTODEPENDENCIES =
@AUTODEPENDENCIES@

t1cs.o: t1cs.hh t1cs.cc
t1interp.o: t1interp.hh t1interp.cc
t1item.o: t1item.hh t1item.cc
t1font.o: t1font.hh t1font.cc
t1mm.o: t1mm.hh t1mm.cc
t1rw.o: t1rw.hh t1rw.cc
t1unparser.o: t1interp.hh t1unparser.hh t1unparser.cc
psres.o: psres.hh psres.cc
