export DYLD_LIBRARY_PATH="$bundle_lib"

# XDG
export XDG_CONFIG_DIRS="$bundle_etc/xdg"
export XDG_DATA_DIRS="$bundle_data"

# Gst env
export GST_PLUGIN_PATH="$bundle_lib/gstreamer-0.10"
export GST_PLUGIN_SYSTEM_PATH="$bundle_lib/gstreamer-0.10"
export GST_REGISTRY_UPDATE="no"

#export GST_DEBUG="*:5"
#export GST_DEBUG_NO_COLOR=1

# Pango env
export PANGO_RC_FILE="$bundle_etc/pango/pangorc"
cat > "$PANGO_RC_FILE" <<EOF
[Pango]
ModuleFiles=$bundle_etc/pango/pango.modules
EOF

cat $bundle_etc/pango/pango.modules | sed -e "s,^.*\(/lib/pango/1.6.0/*\),$bundle_res\1,g" > $bundle_etc/pango/pango.modules.tmp
if [ $? -eq 0 ]; then
	mv $bundle_etc/pango/pango.modules.tmp  $bundle_etc/pango/pango.modules
fi

# gtk/gdk
export GDK_PIXBUF_MODULEDIR="$bundle_lib/gtk-2.0/2.10.0/loaders"
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"

export GTK_DATA_PREFIX="$bundle_res"
export GTK_EXE_PREFIX="$bundle_res"
export GTK_PATH="$bundle_res"

export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"
export GTK2_RC_FILES="$bundle_etc/gtk-2.0/gtkrc"

# Eina env
export EINA_UI_DIR="$bundle_data/eina/ui"
export EINA_PIXMAPS_DIR="$bundle_data/eina/pixmaps"
export EINA_THEME_DIR="$bundle_data/eina/icons"
export EINA_PLUGINS_PATH="$bundle_lib/eina"

