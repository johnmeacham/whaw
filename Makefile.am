AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS=-I ac-macros

bin_PROGRAMS = whaw
man_MANS = whaw.1
whaw_SOURCES = whaw.c  property.c property.h util.h xatoms.c xatoms.h event.c event.h xutil.c xutil.h print_event.c print_event.h

whaw_CFLAGS= -std=c99 @X_CFLAGS@
whaw_LDADD= @X_LIBS@  -lX11 -lXmu -lXinerama @X_EXTRA_LIBS@

EXTRA_DIST = genatoms.prl  xatoms.atoms whaw.pod whaw.1 README.mkd whaw.spec

CLEANFILES=
MAINTAINERCLEANFILES= xatoms.c xatoms.h

BUILT_SOURCES = xatoms.c xatoms.h


xatoms.c: xatoms.atoms genatoms.prl
	perl @srcdir@/genatoms.prl < @srcdir@/xatoms.atoms || rm -f -- xatoms.h xatoms.c
xatoms.h: xatoms.atoms genatoms.prl
	perl @srcdir@/genatoms.prl < @srcdir@/xatoms.atoms || rm -f -- xatoms.h xatoms.c


@PACKAGE@.1: @PACKAGE@.pod
	pod2man -s 1 -r @VERSION@ -c @PACKAGE@  @srcdir@/@PACKAGE@.pod > @PACKAGE@.1

@PACKAGE@.html: @PACKAGE@.pod
	pod2html  --noindex --norecurse @srcdir@/@PACKAGE@.pod > @PACKAGE@.html

README.mkd: @PACKAGE@.pod
	pod2markdown @srcdir@/@PACKAGE@.pod > $@

WEBROOT= ${HOME}/public_html
PUBLISH_DIR = ${WEBROOT}/computer/@PACKAGE@
RPMROOT= ${HOME}/var/rpm

publish: README.mkd whaw.1 whaw.html distcheck
	mkdir -p $(PUBLISH_DIR)
#      mkdir -p $(PUBLISH_DIR)/drop
#	cp @PACKAGE@-@VERSION@.tar.gz  $(PUBLISH_DIR)/drop
	cp @PACKAGE@-@VERSION@.tar.gz  $(WEBROOT)/dist
	cp @PACKAGE@.html $(PUBLISH_DIR)
	darcs changes > Changelog.txt
	cp Changelog.txt $(PUBLISH_DIR)
	make -C $(PUBLISH_DIR) || true
#	rpmbuild -ta @PACKAGE@-@VERSION@.tar.gz
#	cp ${RPMROOT}/SRPMS/@PACKAGE@-@VERSION@-*.rpm ${WEBROOT}/yum
#	cp ${RPMROOT}/RPMS/*/@PACKAGE@-@VERSION@-*.rpm ${WEBROOT}/yum







