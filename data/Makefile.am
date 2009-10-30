BUILT_SOURCES = lomo-player-glue.h
CLEAN_FILES = lomo-player-glue.h
EXTRA_DIST = lomo-player-glue.h

lomo-player-glue.h: lomo-player.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=lomo_player $< > $@

