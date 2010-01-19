DIST_SUBDIRS = adb callhome coverplus fieshta hooktest lastfm ntfy recently

# Stable, basic plugins
SUBDIRS = coverplus lastfm

if BUILD_ADB
SUBDIRS += adb
endif

if BUILD_CALLHOME
SUBDIRS += callhome
endif

# Experimental plugins
if BUILD_EXPERIMENTAL

SUBDIRS += fieshta

if BUILD_ADB
SUBDIRS += recently
endif

if BUILD_NTFY
SUBDIRS += ntfy
endif

endif


