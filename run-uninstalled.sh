#!/bin/bash

export LD_LIBRARY_PATH="`dirname $0`/gel/.libs:`dirname $0`/lomo/.libs"
export DYLD_LIBRARY_PATH="`dirname $0`/gel/.libs:`dirname $0`/lomo/.libs"

# Resource handling (gel-related)
export EINA_UI_PATH="`dirname $0`/ui"
export EINA_PIXMAP_PATH="`dirname $0`/pixmaps"
export EINA_LIB_PATH="`dirname $0`/tools/plugins"

# Eina specific
export EINA_THEME_DIR="`dirname $0`/icons"

if [ ! -z "$(which glib-compile-schemas)" ]; then
	export GSETTINGS_SCHEMA_DIR="`dirname $0`/data"
	glib-compile-schemas "$GSETTINGS_SCHEMA_DIR"
fi

rm -rf -- "`dirname $0`/tools/plugins"
mkdir -p "`dirname $0`/tools/plugins"
for PLUGIN_DIR in $(find "`dirname $0`/plugins" -maxdepth 1 -type d 2>/dev/null | tail -n +2 | grep -v svn)
do
	D="$(dirname -- $0)/tools/plugins"
	B="$(basename -- $PLUGIN_DIR)"
	[ -e "$PLUGIN_DIR/.libs/lib$B.so" ] || continue
	mkdir -p "$D/$B"
	for SO in $(find $PLUGIN_DIR/.libs -type f -name '*.so' 2>/dev/null)
	do
		ln -fs "../../../plugins/$B/.libs/$(basename -- "$SO")" "$D/$B"
	done
	for F in $(find $PLUGIN_DIR -maxdepth 1 | tail -n +2)
	do
		ln -fs "../../../plugins/$B/$(basename -- "$F")" "$D/$B"
	done
done

BIN=""
for i in eina/eina eina/.libs/eina
do
	MIME="$(file -ib "$i" 2>&1 | cut -d ";" -f 1)"
	if [ \
		\( "$MIME" = "application/octet-stream" \) -o \
		\( "$MIME" = "application/x-executable" \) \
		]; then
			BIN="$i"
			break
	fi
done
if [ -z "$BIN" ]; then
	echo "No binary found"
	exit 1
fi

if [ ! -z "$1" ]; then
	case "$1" in
		ltrace)
			shift
			ltrace "`dirname $0`/$BIN" "$@"
			;;
		strace)
			shift
			strace "`dirname $0`/$BIN" "$@"
			;;
		gdb)
			shift
			gdb --args "`dirname $0`/$BIN" "$@"
			;;
		*)
			"`dirname $0`/$BIN" "$@"
			;;
	esac
else 
	"`dirname $0`/$BIN" "$@"
fi

