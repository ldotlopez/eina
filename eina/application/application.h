#ifndef _APPLICATION
#define _APPLICATION

#include <eina/eina-plugin2.h>
#include <gel/gel-ui.h>

#define gel_plugin_engine_get_application(engine) gel_plugin_engine_get_interface(engine, "application")
#define eina_plugin_get_application(plugin) gel_plugin_engine_get_application(gel_plugin_get_engine(plugin))

#endif
