#!/bin/bash

export LD_LIBRARY_PATH="`dirname $0`/gel/.libs:`dirname $0`/lomo/.libs"
export DYLD_LIBRARY_PATH="`dirname $0`/gel/.libs:`dirname $0`/lomo/.libs"

# Resource handling (gel-related)
export EINA_UI_PATH="`dirname $0`/ui"
export EINA_PIXMAP_PATH="`dirname $0`/pixmaps"
export EINA_LIB_PATH="`dirname $0`/tools/plugins"

# Eina specific
export EINA_THEME_DIR="`dirname $0`/icons"

rm -rf -- "`dirname $0`/tools/plugins"
mkdir -p "`dirname $0`/tools/plugins"
for PLUGIN_DIR in $(find "`dirname $0`/plugins" -maxdepth 1 -type d 2>/dev/null | tail -n +2 | grep -v svn)
do
	D="$(dirname -- $0)/tools/plugins"
	B="$(basename -- $PLUGIN_DIR)"
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

if [ ! -z "$1" ]; then
	case "$1" in
		ltrace)
			shift
			ltrace "`dirname $0`/eina/.libs/eina" "$@"
			;;
		strace)
			shift
			strace "`dirname $0`/eina/.libs/eina" "$@"
			;;
		gdb)
			shift
			gdb --args "`dirname $0`/eina/.libs/eina" "$@"
			;;
		*)
			"`dirname $0`/eina/eina" "$@"
			;;
	esac
else 
	"`dirname $0`/eina/eina" "$@"
fi

