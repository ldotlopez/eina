# Build / clean / extra
BUILT_SOURCES = lomo-player-glue.h
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = lomo-player.xml

lomo-player-glue.h: lomo-player.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=lomo_player $< > $@

# pkgconfig files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = eina-@EINA_API_VERSION@.pc

# desktop files
applicationsdir = $(datadir)/applications
applications_DATA = eina.desktop

