#ifndef _EINA_PLUGIN2_H
#define _EINA_PLUGIN2_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-stock.h>

// Redefine some types and enums
#define EinaPluginInfo GelPluginInfo
#define EinaPlugin     GelPlugin
#define EINA_PLUGIN_SERIAL GEL_PLUGIN_SERIAL
#define EINA_PLUGIN_GENERIC_AUTHOR "Luis Lopez <luis.lopez@cuarentaydos.com>"
#define EINA_PLUGIN_GENERIC_URL    "http://eina.sourceforge.net/"
#define EINA_PLUGIN(p)      GEL_PLUGIN(p)

// If EINA_PLUGIN_DATA_TYPE is defined create a macro to easy access
#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) gel_plugin_get_data((GelPlugin *) p))
#endif

// Define a macro for define plugin struct easily
#define EINA_PLUGIN_INFO_SPEC(name,version,deps,author,url,short_desc,long_desc,icon) \
	G_MODULE_EXPORT EinaPluginInfo name ## _plugin_info = { \
		G_STRINGIFY(name), NULL, NULL,                \
                                                      \
		version ? version : PACKAGE_VERSION,          \
		deps,                                         \
		author ? author : EINA_PLUGIN_GENERIC_AUTHOR, \
		url    ? url    : EINA_PLUGIN_GENERIC_URL,    \
                                                      \
		short_desc, long_desc, icon                   \
	}


#endif
