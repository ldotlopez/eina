lib_LTLIBRARIES = libeinaext.la

AM_CPPFLAGS          = @PACKAGE_CFLAGS@
libeinaext_la_LIBADD = @PACKAGE_LIBS@ 

INST_H_FILES = \
	curl-engine.h              \
	eina-conf.h                \
	eina-file-chooser-dialog.h \
	eina-plugin-properties.h   \
	eina-preferences-dialog.h  \
	eina-seek.h                \
	eina-volume.h              \
	eina-window.h

headerdir = $(prefix)/include/eina-@EINA_API_VERSION@/eina/ext
header_DATA = $(INST_H_FILES)

libeinaext_la_SOURCES = \
	curl-engine.c              \
	eina-conf-file.c           \
	eina-file-chooser-dialog.c \
	eina-plugin-properties.c   \
	eina-preferences-dialog.c  \
	eina-seek.c                \
	eina-volume.c              \
	eina-window.c              \
	$(INST_H_FILES)

BUILT_SOURCES = eina-plugin-properties-ui.h
CLEAN_FILES   = eina-plugin-properties-ui.h
EXTRA_DIST    =  eina-plugin-properties.ui

eina-plugin-properties-ui.h: eina-plugin-properties.ui
	../../tools/ui2h eina-plugin-properties.ui ui_xml > eina-plugin-properties-ui.h

if USE_CLUTTER
libeinaext_la_SOURCES += eina-clutty.c
INST_H_FILES += eina-clutty.h
endif

