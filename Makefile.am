## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign check-news

SUBDIRS = liblcdf libefont @SELECTED_SUBDIRS@
DIST_SUBDIRS = liblcdf libefont cfftot1 mmafm mmpfb otftotfm t1lint t1testpage

pkgdata_DATA = glyphlist.txt 7t.enc

EXTRA_DIST = \
	ONEWS \
	include/lcdf/bezier.hh \
	include/lcdf/clp.h \
	include/lcdf/error.hh \
	include/lcdf/filename.hh \
	include/lcdf/hashmap.hh include/lcdf/hashmap.cc \
	include/lcdf/inttypes.h \
	include/lcdf/landmark.hh \
	include/lcdf/permstr.hh \
	include/lcdf/point.hh \
	include/lcdf/slurper.hh \
	include/lcdf/straccum.hh \
	include/lcdf/string.hh \
	include/lcdf/strtonum.h \
	include/lcdf/transform.hh \
	include/lcdf/vector.hh include/lcdf/vector.cc \
	include/efont/afm.hh \
	include/efont/afmparse.hh \
	include/efont/afmw.hh \
	include/efont/amfm.hh \
	include/efont/cff.hh \
	include/efont/encoding.hh \
	include/efont/findmet.hh \
	include/efont/metrics.hh \
	include/efont/otf.hh \
	include/efont/otfcmap.hh \
	include/efont/otfdata.hh \
	include/efont/otfgpos.hh \
	include/efont/otfgsub.hh \
	include/efont/pairop.hh \
	include/efont/psres.hh \
	include/efont/t1bounds.hh \
	include/efont/t1cs.hh \
	include/efont/t1csgen.hh \
	include/efont/t1font.hh \
	include/efont/t1interp.hh \
	include/efont/t1item.hh \
	include/efont/t1mm.hh \
	include/efont/t1rw.hh \
	include/efont/t1unparser.hh \
	glyphlist.txt \
	7t.enc

srclinks:
	cd liblcdf && $(MAKE) srclinks
	cd libefont && $(MAKE) srclinks

versionize:
	perl -pi -e 's/^\.ds V.*/.ds V $(VERSION)/;' $(top_srcdir)/cfftot1/cfftot1.1 $(top_srcdir)/mmafm/mmafm.1 $(top_srcdir)/mmpfb/mmpfb.1 $(top_srcdir)/otftotfm/otftotfm.1 $(top_srcdir)/t1lint/t1lint.1
	perl -pi -e 's/^(\U$(PACKAGE)\E) [\d.ab]+$$/$$1 $(VERSION)/;' $(top_srcdir)/README

.PHONY: srclinks rpm
