AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBLOMO_COMPILATION \
	@PACKAGE_CFLAGS@

lib_LTLIBRARIES = liblomo-1.0.la
liblomo_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0
liblomo_1_0_la_LIBADD = @PACKAGE_LIBS@ $(INTLLIBS)

NO_INST_H_FILES = \
	lomo-marshallers.h           \
	lomo-metadata-parser.h       \
	lomo-playlist.h

INST_H_FILES = \
	lomo-player.h           \
	lomo-metadata-parser.h  \
	lomo-stream.h           \
	lomo-util.h

headerdir = $(prefix)/include/@PACKAGE@-@EINA_API_VERSION@/lomo
header_DATA = $(INST_H_FILES)

liblomo_1_0_la_SOURCES = \
	lomo-marshallers.c           \
	lomo-metadata-parser.c       \
	lomo-player.c                \
	lomo-playlist.c              \
	lomo-stream.c                \
	lomo-util.c                  \
	$(INST_H_FILES)              \
	$(NO_INST_H_FILES)

BUILT_SOURCES = lomo-marshallers.c lomo-marshallers.h
CLEANFILES = $(BUILT_SOURCES)

lomo-marshallers.c: lomo-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=lomo_marshal --header --body > $@

lomo-marshallers.h: lomo-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=lomo_marshal --header > $@

EXTRA_DIST = lomo-marshallers.list
