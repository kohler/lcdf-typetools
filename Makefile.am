## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = no-dependencies foreign check-news

SUBDIRS = edlib t1lib t1lint

EXTRA_DIST = rpm.spec
AUTODEPENDENCIES =

srclinks:
	for i in clp.h clp.c hashmap.hh hashmap.cc vector.hh vector.cc \
	permstr.hh permstr.cc straccum.hh straccum.cc string.hh string.cc \
	landmark.hh landmark.cc error.hh error.cc filename.hh filename.cc \
	slurper.hh slurper.cc strtonum.h strtonum.c fixlibc.c ; do \
	ln -sf ../../edlib/$$i edlib/$$i ; done
	for i in t1cs.hh t1cs.cc t1interp.hh t1interp.cc t1mm.hh t1mm.cc \
	t1item.hh t1item.cc t1font.hh t1font.cc t1rw.hh t1rw.cc \
	t1unparser.hh t1unparser.cc psres.hh psres.cc ; do \
	ln -sf ../../t1lib/$$i t1lib/$$i ; done

versionize:
	perl -pi -e "s/^\\.ds V.*/.ds V $(VERSION)/;" mmafm/mmafm.1 mmpfb/mmpfb.1
	perl -pi -e "s/^Version: .*/Version: $(VERSION)/; s/$(PACKAGE)-[\w.]+\.tar\.gz/$(PACKAGE)-$(VERSION).tar.gz/;" rpm.spec

rpm: dist
	buildarch=`rpm --showrc | awk '/^build arch/ { print $$4; }'` ; \
	mkdir -p /tmp/rpmb/SOURCES /tmp/rpmb/RPMS/$$buildarch \
	/tmp/rpmb/BUILD ; \
	echo 'topdir: /tmp/rpmb' > /tmp/rpmb/rc ; \
	cp logo1.gif $(PACKAGE)-$(VERSION).tar.gz /tmp/rpmb/SOURCES ; \
	rpm --rcfile /tmp/rpmb/rc -bb rpm.spec ; \
	cp /tmp/rpmb/RPMS/$$buildarch/*.rpm .
	rm -rf /tmp/rpmb

.PHONY: srclinks versionize rpm
