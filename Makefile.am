## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign check-news

SUBDIRS = liblcdf libefont cfftot1 t1lint t1testpage

EXTRA_DIST = rpm.spec
AUTODEPENDENCIES =

srclinks:
	test $(srcdir) = .
	cd liblcdf && $(MAKE) srclinks
	cd libefont && $(MAKE) srclinks

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
