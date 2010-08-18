include $(top_srcdir)/build/Makefile.am.common

SUBDIRS = ext

bin_PROGRAMS = eina-gtk3
eina_gtk3_LDDADD = $(top_builddir)/eina/ext/.libs/libeinaext.la
eina_gtk3_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
eina_gtk3_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0

eina_gtk3_SOURCES = \
	fs.c \
	fs.h \
	eina-app.c
#	ext/eina-file-utils.c \
#	ext/eina-file-utils.h \
#	ext/eina-stock.c \
#	ext/eina-stock.h \
#	ext/eina-file-chooser-dialog.c \
#	ext/eina-file-chooser-dialog.h \
#	art.c \
#	art.h

