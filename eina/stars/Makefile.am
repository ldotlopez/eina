-include $(top_srcdir)/git.mk
include $(top_srcdir)/build/Makefile.am.common

plugindir = $(pkglibdir)/stars
plugin_PYTHON = stars.py widgets.py

plugin_DATA = stars.plugin \
	dock.ui     \
	star-on.svg \
	star-off.svg
EXTRA_DIST += $(plugin_DATA)

-include $(top_srcdir)/git.mk
