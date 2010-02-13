noinst_LTLIBRARIES = libeinaext.la

AM_CPPFLAGS          = @PACKAGE_CFLAGS@
libeinaext_la_LIBADD = @PACKAGE_LIBS@ 

INST_H_FILES = \
	curl-engine.h              \
	eina-conf.h                \
	eina-file-chooser-dialog.h \
	eina-plugin-dialog.h       \
	eina-plugin-properties.h   \
	eina-preferences-dialog.h  \
	eina-seek.h                \
	eina-stock.h               \
	eina-volume.h              \
	eina-window.h

NO_INST_H_FILES = \
	eina-preferences-dialog-marshallers.h

headerdir = $(prefix)/include/@PACKAGE_NAME@-@EINA_API_VERSION@/eina/ext
header_DATA = $(INST_H_FILES)

libeinaext_la_SOURCES = \
	curl-engine.c              \
	eina-conf-file.c           \
	eina-file-chooser-dialog.c \
	eina-plugin-dialog.c       \
	eina-plugin-properties.c   \
	eina-preferences-dialog.c  \
	eina-preferences-dialog-marshallers.c  \
	eina-seek.c                \
	eina-stock.c               \
	eina-volume.c              \
	eina-window.c              \
	$(INST_H_FILES)            \
	$(NO_INST_H_FILES)

BUILT_SOURCES = \
	eina-plugin-dialog-ui.h             \
	eina-plugin-properties-ui.h         \
	eina-preferences-dialog-marshallers.c eina-preferences-dialog-marshallers.h

CLEAN_FILES   = \
	eina-plugin-dialog-ui.h             \
	eina-plugin-properties-ui.h         \
	eina-preferences-dialog-marshallers.c eina-preferences-dialog-marshallers.h

EXTRA_DIST    = \
	eina-plugin-dialog.ui     \
	eina-plugin-properties.ui \
	eina-preferences-dialog-marshallers.list

eina-preferences-dialog-marshallers.h: eina-preferences-dialog-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_preferences_dialog_marshal --header > $@

eina-preferences-dialog-marshallers.c: eina-preferences-dialog-marshallers.list
	$(GLIB_GENMARSHAL) $< --prefix=eina_preferences_dialog_marshal --header --body > $@


eina-plugin-dialog-ui.h: eina-plugin-dialog.ui
	../../tools/ui2h eina-plugin-dialog.ui ui_xml > eina-plugin-dialog-ui.h

eina-plugin-properties-ui.h: eina-plugin-properties.ui
	../../tools/ui2h eina-plugin-properties.ui ui_xml > eina-plugin-properties-ui.h

if USE_CLUTTER
libeinaext_la_SOURCES += eina-clutty.c
INST_H_FILES += eina-clutty.h
endif

