## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

noinst_LIBRARIES = liblcdf.a

liblcdf_a_SOURCES = clp.c \
	error.cc \
	filename.cc \
	landmark.cc \
	permstr.cc \
	slurper.cc \
	straccum.cc \
	string.cc \
	strtonum.c \
	vectorv.cc

liblcdf_a_DEPENDENCIES = @FIXLIBC_O@
liblcdf_a_LIBADD = @FIXLIBC_O@

INCLUDES = -I$(top_srcdir)/include

EXTRA_DIST = fixlibc.c

srclinks:
	test -d $(top_srcdir)/include || mkdir $(top_srcdir)/include
	test -d $(top_srcdir)/include/lcdf || mkdir $(top_srcdir)/include/lcdf
	for i in permstr straccum string landmark error filename slurper ; do \
	ln -sf ../../liblcdf/liblcdf/$$i.cc $$i.cc ; \
	ln -sf ../../../liblcdf/include/lcdf/$$i.hh ../include/lcdf/$$i.hh ; done
	for i in clp strtonum ; do \
	ln -sf ../../liblcdf/liblcdf/$$i.c $$i.c ; \
	ln -sf ../../../liblcdf/include/lcdf/$$i.h ../include/lcdf/$$i.h ; done
	for i in fixlibc.c vectorv.cc ; do \
	ln -sf ../../liblcdf/liblcdf/$$i $$i ; done
	for i in hashmap vector ; do \
	ln -sf ../../../liblcdf/include/lcdf/$$i.cc ../include/lcdf/$$i.cc ; \
	ln -sf ../../../liblcdf/include/lcdf/$$i.hh ../include/lcdf/$$i.hh ; done
