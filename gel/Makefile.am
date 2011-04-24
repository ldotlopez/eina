include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = libgel-1.1.la libgel-ui-1.1.la

libgel_1_1_la_CFLAGS  = -DGEL_COMPILATION @GLIB_CFLAGS@ @GTK_CFLAGS@ @DEBUG_CFLAGS@
libgel_1_1_la_LDFLAGS = -export-dynamic -version-info 1:1:1 -export-dynamic @GLIB_LIBS@

libgel_ui_1_1_la_CFLAGS  = -DGEL_UI_COMPILATION @GLIB_CFLAGS@ @GTK_CFLAGS@
libgel_ui_1_1_la_LDFLAGS = $(libgel_1_1_la_LDFLAGS) @GTK_LIBS@

common_includedir = $(prefix)/include/gel-@GEL_API_VERSION@/gel

libgel_includedir = $(common_includedir)
libgel_include_HEADERS = \
	gel.h                   \
	gel-misc.h              \
	gel-string.h            \
	gel-str-parser.h        \
	gel-job-queue.h         \
	gel-plugin.h            \
	gel-plugin-engine.h     \
	gel-plugin-info.h       \
	gel-io.h                \
	gel-io-scanner.h

libgel_ui_includedir = $(common_includedir)
libgel_ui_include_HEADERS = \
	gel-ui.h                \
	gel-ui-dialogs.h        \
	gel-ui-generic.h        \
	gel-ui-plugin-manager.h \
	gel-ui-utils.h

common_geldir = $(libdir)/gel-@GEL_API_VERSION@

libgeldir = $(common_geldir)
libgel_1_1_la_SOURCES = \
	$(libgel_include_HEADERS) \
	gel-misc.c                \
	gel-string.c              \
	gel-str-parser.c          \
	gel-job-queue.c           \
	gel-marshallers.c         \
	gel-plugin.c              \
	gel-plugin-engine.c       \
	gel-plugin-info.c         \
	gel-io-scanner.c

libgel_uidir = $(common_geldir)
libgel_ui_1_1_la_SOURCES = \
	$(libgel_ui_include_HEADERS) \
	gel-ui-dialogs.c             \
	gel-ui-generic.c             \
	gel-ui-plugin-manager.c      \
	gel-ui-plugin-manager-ui.h   \
	gel-ui-utils.c

glib_marshallers_list = gel-marshallers.list
glib_marshallers_prefix = gel_marshal
include $(top_srcdir)/build/Makefile.am.marshallers

eina_ui2h_ui = gel-ui-plugin-manager.ui
eina_ui2h_name = __gel_ui_plugin_manager_ui_xml
include $(top_srcdir)/build/Makefile.am.ui2h

-include $(top_srcdir)/git.mk

