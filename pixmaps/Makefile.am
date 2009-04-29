themedir = $(pkgdatadir)/icons/hicolor

# 16x16 22x22 24x24 32x32 36x36 48x48 64x64 72x72 96x96 128x128 192x192 scalable

t16actions = 16x16/actions
t22actions = 22x22/actions
t24actions = 24x24/actions

theme16actionsdir = $(themedir)/$(t16actions)
theme16actions_DATA = $(t16actions)/queue.png

theme22actionsdir = $(themedir)/$(t22actions)
theme22actions_DATA = $(t22actions)/queue.png

theme24actionsdir = $(themedir)/$(t24actions)
theme24actions_DATA = $(t24actions)/queue.png

#icons_DATA = \
#	$(t16actions)/queue.png \
#	$(t22actions)/queue.png \
#	$(t24actions)/queue.png

# :r ! /bin/ls -1 | grep -v Makefile | sed -e 's/^\(.*\)$/\t\1 \\/'
pixmapsdir = $(pkgdatadir)/pixmaps
pixmaps_DATA = \
	cover-default.png \
	cover-loading.png \
	cover-mask.png \
	file-chooser-loading.gif \
	icon-512.png \
	icon.png \
	logo.png \
	random.png \
	repeat.png \
	standard-systray-icon.png \
	osx-systray-icon.png

EXTRA_DIST = \
	$(icons_DATA) \
	$(pixmaps_DATA)

