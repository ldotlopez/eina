include $(top_srcdir)/build/Makefile.am.common

# :r ! /bin/ls -1 | grep -v Makefile | sed -e 's/^\(.*\)$/\t\1 \\/'
pixmapsdir = $(pkgdatadir)/pixmaps
pixmaps_DATA =  \
	cover-default.png      \
	cover-loading.png      \
	eina.svg               \
	loading-spin-16x16.gif \
	plugin.png
EXTRA_DIST += $(pixmaps_DATA)


-include $(top_srcdir)/git.mk
