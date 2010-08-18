include $(top_srcdir)/Makefile.am.common

lib_LTLIBRARIES = libgel-1.0.la
libgel_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0 -export-dynamic @GLIB_LIBS@ @GTK_LIBS@
libgel_1_0_la_CFLAGS = -DGEL_COMPILATION @GLIB_CFLAGS@ @GTK_CFLAGS@
geldir = $(prefix)/lib

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/gel
include_HEADERS = \
	gel.h                   \
	gel-misc.h              \
	gel-string.h            \
	gel-str-parser.h        \
	gel-job-queue.h         \
	gel-plugin.h            \
	gel-plugin-engine.h     \
	gel-plugin-info.h       \
	gel-io-misc.h           \
	gel-io-tree.h           \
	gel-io-op-result.h      \
	gel-io-ops.h            \
	gel-io-recurse-tree.h   \
	gel-io-scanner.h        \
	gel-io.h                \
	gel-ui.h                \
	gel-ui-application.h    \
	gel-ui-generic.h        \
	gel-ui-plugin-manager.h \
	gel-ui-utils.h

libgel_1_0_la_SOURCES = \
	gel-misc.c              \
	gel-string.c            \
	gel-str-parser.c        \
	gel-job-queue.c         \
	gel-marshallers.c       \
	gel-plugin.c            \
	gel-plugin-engine.c     \
	gel-plugin-info.c       \
	gel-io-misc.c           \
	gel-io-tree.c           \
	gel-io-op-result.c      \
	gel-io-ops.c            \
	gel-io-recurse-tree.c   \
	gel-io-scanner.c        \
	gel-plugin-engine.h     \
	gel-ui-application.c    \
	gel-ui-generic.c        \
	gel-ui-plugin-manager.c \
	gel-ui-utils.c

glib_marshallers_list = gel-marshallers.list
glib_marshallers_prefix = gel_marshal
include $(top_srcdir)/Makefile.am.marshallers

eina_ui2h_ui = gel-ui-plugin-manager.ui
eina_ui2h_name = __gel_ui_plugin_manager_ui_xml
include $(top_srcdir)/Makefile.am.ui2h


