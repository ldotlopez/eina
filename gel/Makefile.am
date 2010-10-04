include $(top_srcdir)/build/Makefile.am.common

lib_LTLIBRARIES = libgel-1.0.la
libgel_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0 -export-dynamic @GLIB_LIBS@ @GTK_LIBS@
libgel_1_0_la_CFLAGS = -DGEL_COMPILATION @GLIB_CFLAGS@ @GTK_CFLAGS@ @DEBUG_CFLAGS@
geldir = $(prefix)/lib

includedir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/gel
include_HEADERS = \
	gel.h                   \
	gel-app.h               \
	gel-io-misc.h           \
	gel-io-tree.h           \
	gel-io-op-result.h      \
	gel-io-ops.h            \
	gel-io-recurse-tree.h   \
	gel-io-scanner.h        \
	gel-io.h                \
	gel-job-queue.h         \
	gel-misc.h              \
	gel-plugin.h            \
	gel-string.h            \
	gel-str-parser.h        \
	gel-ui.h

libgel_1_0_la_SOURCES = \
	gel-app.c               \
	gel-io-misc.c           \
	gel-io-tree.c           \
	gel-io-op-result.c      \
	gel-io-ops.c            \
	gel-io-recurse-tree.c   \
	gel-io-scanner.c        \
	gel-job-queue.c         \
	gel-marshallers.h       \
	gel-marshallers.c       \
	gel-misc.c              \
	gel-string.c            \
	gel-str-parser.c        \
	gel-plugin.c            \
	gel-ui.c

glib_marshallers_list = gel-marshallers.list
glib_marshallers_prefix = gel_marshal
include $(top_srcdir)/build/Makefile.am.marshallers

