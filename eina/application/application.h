#ifndef _APPLICATION
#define _APPLICATION

#include <eina/eina-plugin2.h>
#include <gel/gel-ui.h>

// Accessors
#define gel_plugin_engine_get_application(engine) gel_plugin_engine_get_interface(engine, "application")
#define eina_plugin_get_application(plugin)       gel_plugin_engine_get_application(gel_plugin_get_engine(plugin))

// Plugin API
#define eina_plugin_get_application_window(plugin) gel_ui_application_get_window(eina_plugin_get_application(plugin))

#endif
