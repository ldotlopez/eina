INCLUDES = \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-DLIBLOMO_COMPILATION

lib_LTLIBRARIES = liblomo-1.0.la
liblomo_1_0_la_LDFLAGS = -export-dynamic -version-info 1:0:0
liblomo_1_0_la_LIBADD = @PACKAGE_LIBS@ $(INTLLIBS)

NO_INST_H_FILES =   \
	lomo-marshal.h  \
	meta.h          \
	pl.h            \
	player-priv.h   \
	player-defaul-vtable.h

INST_H_FILES = \
	player.h   \
	stream.h   \
	util.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/lomo
header_DATA = $(INST_H_FILES)

liblomo_1_0_la_SOURCES = \
	lomo-marshal.c     \
	meta.c             \
	pl.c               \
	player-default-vtable.c \
	player-priv.c      \
	player.c           \
	stream.c           \
	util.c             \
	$(INST_H_FILES)    \
	$(NO_INST_H_FILES)

BUILT_SOURCES = lomo-marshal.c lomo-marshal.h
CLEANFILES = $(BUILT_SOURCES)

lomo-marshal.c: lomo-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=lomo_marshal --header --body > $@

lomo-marshal.h: lomo-marshal.list
	$(GLIB_GENMARSHAL) $< --prefix=lomo_marshal --header > $@

EXTRA_DIST = lomo-marshal.list
