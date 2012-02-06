#!/bin/bash

if test "x$GTK_DEBUG_LAUNCHER" != x; then
	set -x
fi

if test "x$GTK_DEBUG_GDB" != x; then
	EXEC="gdb --args"
elif test "x$GTK_DEBUG_DTRUSS" != x; then
	EXEC="dtruss"
else
	EXEC=exec
fi

name=$(basename "$0")
dirn=$(dirname "$0")

bundle=$(cd "$dirn/../../" && pwd)
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export PATH="$bundle_bin:$PATH"
export DYLD_LIBRARY_PATH="$bundle_lib:$DYLD_LIBRARY_PATH"
export XDG_CONFIG_DIRS="$bundle_etc:$XDG_CONFIG_DIRS"
export XDG_DATA_DIRS="$bundle_data:$XDG_DATA_DIRS"
export GTK_DATA_PREFIX="$bundle_res"
export GTK_EXE_PREFIX="$bundle_res"
export GTK_PATH="$bundle_res"
export GDK_PIXBUF_MODULE_FILE="$bundle_lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
export GIO_EXTRA_MODULES="$bundle_lib/gio/modules"
export GI_TYPELIB_PATH="$bundle_lib/girepository-1.0"
export PYTHONPATH="$bundle_lib/python2.7/site-packages:$PYTHONPATH"
export PYTHONHOME="$bundle_res"
export PANGO_LIBDIR="$bundle_lib"
export PANGO_SYSCONFDIR="$bundle_etc"
export PEAS_PLUGIN_LOADERS_DIR="$bundle_lib/libpeas-1.0/loaders"
export DBUS_REPLACE_INSTALL_PREFIX=$(printf '%q' "$bundle_res/")

export EINA_LIB_PATH="$bundle_lib/eina"
export EINA_PIXMAP_PATH="$bundle_data/eina/pixmaps"
export EINA_THEME_DIR="$bundle_data/eina/icons"
export GST_PLUGIN_SYSTEM_PATH="$bundle_lib/gstreamer-0.10"
export GST_REGISTRY_FORK="no"

# Eina specific
#export EINA_DESKTOP_FILE="$D/data/eina.desktop"

if test -f "$bundle_lib/charset.alias"; then
	export CHARSETALIASDIR="$bundle_lib"
fi

# Extra arguments can be added in environment.sh.
EXTRA_ARGS=
if test -f "$bundle_res/environment.sh"; then
	source "$bundle_res/environment.sh"
fi

if test -f "$HOME/.einaenv"; then
	source "$HOME/.einaenv"
fi

# Strip out the argument added by the OS.
if [ x`echo "x$1" | sed -e "s/^x-psn_.*//"` == x ]; then
	shift 1
fi

# Launch dbus if needed
dbusenv="$TMPDIR/eina-$USER.dbus"

if [ -f "$dbusenv" ]; then
	source "$dbusenv"
fi

if [ -z "$DBUS_SESSION_BUS_PID" ] || ! ps -p "$DBUS_SESSION_BUS_PID" >/dev/null; then
	"$bundle_bin/dbus-launch" --config-file "$bundle_etc/dbus-1/session.conf" > "$dbusenv"

	source "$dbusenv"
fi

export DBUS_SESSION_BUS_PID
export DBUS_SESSION_BUS_ADDRESS

if [ "x$GTK_DEBUG_SHELL" != "x" ]; then
	exec bash
else
	$EXEC "$bundle_contents/MacOS/$name-bin" "$@" $EXTRA_ARGS
fi
