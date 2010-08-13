include $(top_srcdir)/Makefile.am.common

SUBDIRS = ext

bin_PROGRAMS = eina-app
eina_app_CFLAGS = @EINA_CFLAGS@ -DEINA_COMPILATION -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" 
eina_app_LDFLAGS = \
	@EINA_LIBS@ \
	-L$(top_builddir)/gel/.libs  \
	-L$(top_builddir)/lomo/.libs \
	-lgel-1.0 -llomo-1.0

eina_app_SOURCES = \
	ext/eina-file-utils.c \
	ext/eina-file-utils.h \
	ext/eina-stock.c \
	ext/eina-stock.h \
	ext/eina-file-chooser-dialog.c \
	ext/eina-file-chooser-dialog.h \
	fs.c \
	fs.h \
	eina-app.c

