#!/bin/bash

D="$(dirname -- "$0")"
if [ "${D:0:1}" != "/" ]; then
	D="$PWD/$D"
fi

R="$D/tools/run"
[ -e "$R" ] && rm -rf -- "$R"

# Setup path for libs
unset GTK_MODULES
export LD_LIBRARY_PATH="$D/lomo/.libs:$D/gel/.libs:$LD_LIBRARY_PATH"

# Resource handling (gel-related)
# export EINA_UI_PATH="$D/ui"
export EINA_PIXMAP_PATH="$D/pixmaps"
export EINA_LIB_PATH="$R/plugins"

# Eina specific
export EINA_THEME_DIR="$D/icons"

[ -e "$R" ] && rm -rf "$R"

# Schemas
if [ ! -z "$(which glib-compile-schemas)" ]; then
	mkdir -p "$R/schemas"
	for schema in $(find "$D/data" "$D/plugins" -name '*gschema.valid' | sed -e 's,.valid$,.xml,')
	do
		echo "[*] Added schema $(basename -- $schema)"
		ln -s "$schema" "$R/schemas/$(basename -- $schema)"
	done

	export GSETTINGS_SCHEMA_DIR="$R/schemas"
	glib-compile-schemas "$GSETTINGS_SCHEMA_DIR"
fi

# Plugins
IFS="
"
for plugindir in $(find "$D/plugins" -maxdepth 1 -type d | tail -n +2)
do
	plugin=$(basename -- "$plugindir")

	mkdir -p "$R/plugins/$plugin"
	for ext in so ui png jpg gif ini
	do
		for obj in $(find "$plugindir" -name "*.$ext")
		do
			[ "$ext" = "so" ] && echo "[*] Detected plugin $plugin"
			ln -s $obj "$R/plugins/$plugin"
		done
	done
done

# Find real binary
P=${P:="eina-gtk3"}

BIN=""
for i in $(find "$D/eina" -name "$P")
do
	if [ "$(file -ib "$i" | cut -d ';' -f 1)" = "application/x-executable" ]; then
		BIN="$i"
		break;
	fi
done
if [ ! -x  "$BIN" ]; then
	echo "Unable to find executable"
	exit 1
fi
echo "[*] Found binary $BIN"

if [ ! -z "$1" ]; then
	case "$1" in
		noop)
			exit 0
			;;

		ltrace)
			shift
			ltrace "$BIN" "$@"
			;;
		strace)
			shift
			strace "$BIN" "$@"
			;;
		gdb)
			shift
			gdb --args "$BIN" "$@"
			;;
		*)
			"$BIN" "$@"
			;;
	esac
else 
	"$BIN" "$@"
fi

