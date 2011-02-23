include $(top_srcdir)/build/Makefile.am.common

noinst_LTLIBRARIES = libeinaext.la

libeinaext_la_LDFLAGS = @EINA_LIBS@ -L$(top_builddir)/gel/.libs -L$(top_builddir)/lomo/.libs -lgel-ui-@GEL_API_VERSION@ -llomo-@LOMO_API_VERSION@
libeinaext_la_CFLAGS = @EINA_CFLAGS@
includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/eina/ext
include_HEADERS = \
	curl-engine.h              \
	eina-application.h         \
	eina-file-chooser-dialog.h \
	eina-file-utils.h          \
	eina-fs.h                  \
	eina-stock.h               \
	eina-window.h

libeinaext_la_SOURCES = \
	$(include_HEADERS)         \
	eina-application.c         \
	curl-engine.c              \
	eina-file-chooser-dialog.c \
	eina-file-utils.c          \
	eina-fs.c                  \
	eina-stock.c               \
	eina-window.c


-include $(top_srcdir)/build/git.mk
