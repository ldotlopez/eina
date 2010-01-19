DIST_SUBDIRS = adb callhome coverplus fieshta hooktest lastfm ntfy recently

# Stable, basic plugins
SUBDIRS = coverplus lastfm

if USE_ADB
SUBDIRS += adb
endif

if USE_CALLHOME
SUBDIRS += callhome
endif

if USE_NOTIFY
SUBDIRS += ntfy
endif

# Experimental plugins
if USE_EXPERIMENTAL
SUBDIRS += fieshta
if USE_ADB
SUBDIRS += recently
endif
endif


