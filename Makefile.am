ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib python tests

bin_PROGRAMS = btparser
btparser_CFLAGS = -Wall
btparser_SOURCES = btparser.c
btparser_LDADD = lib/libbtparser.la

man_MANS = btparser.1
EXTRA_DIST = btparser.1.in

btparser.1: btparser.1.in
	sed -e "s,\@PACKAGE_STRING\@,$(PACKAGE_STRING),g" $< > $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = btparser.pc
DISTCLEANFILES = $(pkgconfig_DATA)
EXTRA_DIST += btparser.pc.in

EXTRA_DIST += autogen.sh RELEASE

UPLOAD_URI = kklic@fedorahosted.org:btparser
.PHONY: upload
upload:
	make dist-gzip
	scp $(distdir).tar.gz $(UPLOAD_URI)
	make dist-zip
	scp $(distdir).zip $(UPLOAD_URI)
	make dist-xz
	scp $(distdir).tar.xz $(UPLOAD_URI)

RPM_DIRS = --define "_sourcedir `pwd`" \
           --define "_rpmdir `pwd`" \
           --define "_specdir `pwd`" \
           --define "_builddir `pwd`" \
           --define "_srcrpmdir `pwd`"

EXTRA_DIST += btparser.spec.in
rpm: dist-xz btparser.spec
	rpmbuild $(RPM_DIRS) -ba btparser.spec
srpm: dist-xz btparser.spec
	rpmbuild $(RPM_DIRS) -bs btparser.spec
