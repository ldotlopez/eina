#ifndef _EINA_PLUGIN
#define _EINA_PLUGIN

// system libs
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

// liblomo
#include <lomo/player.h>

// libgel
#include <gel/gel.h>
#include <gel/gel-ui.h>

// Modules
#include <eina/player.h>
#include <eina/fs.h>
#include <eina/settings.h>
#include <eina/artwork.h>

// Redefine some types and enums
#define EinaPlugin GelPlugin

#define EINA_PLUGIN_SERIAL GEL_PLUGIN_SERIAL
#define EINA_PLUGIN_GENERIC_AUTHOR "xuzo <xuzo@cuarentaydos.com>"
#define EINA_PLUGIN_GENERIC_URL    "http://eina.sourceforge.net/"
#define EINA_PLUGIN(p)      GEL_PLUGIN(p)

#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) EINA_PLUGIN(p)->data)
#endif


// --
// Access to internal values
// --
#define eina_plugin_get_app(p) \
	gel_plugin_get_app(p)
/*
LomoPlayer*
eina_plugin_get_lomo(EinaPlugin *self);
*/

// --
// Utility functions
// --
gchar*
eina_plugin_build_resource_path(EinaPlugin *plugin, gchar *resource);
gchar*
eina_plugin_build_userdir_path (EinaPlugin *plugin, gchar *path);

#define eina_plugin_verbose(...) _gel_debug(GEL_DEBUG_LEVEL_VERBOSE, __VA_ARGS__)
#define eina_plugin_debug(...)   _gel_debug(GEL_DEBUG_LEVEL_DEBUG,   __VA_ARGS__)
#define eina_plugin_info(...)    _gel_debug(GEL_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#define eina_plugin_warn(...)    _gel_debug(GEL_DEBUG_LEVEL_WARN,    __VA_ARGS__)
#define eina_plugin_error(...)   _gel_debug(GEL_DEBUG_LEVEL_ERROR,   __VA_ARGS__)

// --
// Dock handling (dock is managed by EinaPlugin currently, but there are plans
// to use EinaPlugin in the future, so use eina_plugin_dock_* macros if you
// are writting a plugin)
// --
gboolean
eina_plugin_add_dock_widget(EinaPlugin *self, gchar *id, GtkWidget *label, GtkWidget *widget);

gboolean
eina_plugin_remove_dock_widget(EinaPlugin *self, gchar *id);

gboolean
eina_plugin_switch_dock(EinaPlugin *self, gchar *id);

// --
// Configuration handling
// --
gboolean eina_plugin_add_configuration_widget
(EinaPlugin *plugin, GtkImage *icon, GtkLabel *label, GtkWidget *widget);
gboolean eina_plugin_remove_configuration_widget
(EinaPlugin *plugin,  GtkWidget *widget);

// --
// Artwork handling (cover replacement)
// --
EinaArtwork*
eina_plugin_get_artwork(EinaPlugin *plugin);

void
eina_plugin_add_artwork_provider(EinaPlugin *plugin, gchar *id,
	EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel);
void
eina_plugin_remove_artwork_provider(EinaPlugin *plugin, gchar *id);

// --
// Lomo events
// --
void
eina_plugin_attach_events(EinaPlugin *plugin, ...);
void
eina_plugin_deattach_events(EinaPlugin *plugin, ...);

#endif /* _EINA_PLUGIN */
