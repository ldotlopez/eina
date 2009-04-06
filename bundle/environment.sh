#Gst env
export GST_PLUGIN_PATH=$bundle_res/lib/gstreamer-0.10
export GST_PLUGIN_SYSTEM_PATH=$bundle_res/lib/gstreamer-0.10
export GST_REGISTRY_UPDATE="no"

#Pango env
export PANGO_RC_FILE=$bundle_res/etc/pango/pangorc
cat > $bundle_res/etc/pango/pangorc <<EOF
[Pango]
ModuleFiles=$bundle_res/etc/pango/pango.modules
EOF

cat $bundle_res/etc/pango/pango.modules | sed -e "s,^.*\(/lib/pango/1.6.0/*\),$bundle_res\1,g" > $bundle_res/etc/pango/pango.modules.tmp
if [ $? -eq 0 ]; then
	mv $bundle_res/etc/pango/pango.modules.tmp  $bundle_res/etc/pango/pango.modules
fi

#export GST_DEBUG="*:5"
#export GST_DEBUG_NO_COLOR=1

#Eina env
export EINA_PIXMAPS_DIR=$bundle_res/share/eina/pixmaps
export EINA_UI_DIR=$bundle_res/share/eina/ui
export EINA_PLUGINS_PATH="$bundle_res/lib/eina"
