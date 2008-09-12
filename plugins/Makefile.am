SUBDIRS = hooktest recently

if USE_ADB
SUBDIRS += adb
endif

if USE_LASTFMINFO
SUBDIRS += lastfminfo
endif

if USE_NOTIFY
SUBDIRS += notify
endif

DIST_SUBDIRS = \
	adb \
	hooktest \
	lastfminfo \
	notify \
	recently

#install-data-local:
#	@$(NORMAL_INSTALL)
#	$(mkinstalldirs) $(DESTDIR)$(plugins_data_dir)

