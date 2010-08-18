include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = ext

bin_PROGRAMS = eina-gtk3
eina_gtk3_LDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
eina_gtk3_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
eina_gtk3_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina
include_HEADERS = \
	eina-plugin2.h

eina_gtk3_SOURCES = \
	fs.c \
	fs.h \
	eina-marshallers.c \
	eina-marshallers.h \
	eina-app.c

glib_marshallers_list = eina-marshallers.list
glib_marshallers_prefix = eina_marshall
include $(top_srcdir)/build/Makefile.am.marshallers

