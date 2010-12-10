SUBDIRS = \
	build   \
	gel     \
	lomo    \
	eina    \
	pixmaps \
	data    \
	docs    \
	icons   \
	po

EXTRA_DIST = \
	run-uninstalled.sh \
	m4/introspection.m4 \
	intltool-extract.in intltool-merge.in intltool-update.in \
	AUTHORS BUGS COPYING INSTALL NEWS README

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-instrospection --enable-gtk-doc

