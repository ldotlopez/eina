#ifndef _EINA_PLAGIN
#define _EINA_PLAGIN

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
#include <eina/player2.h>
#include <eina/fs.h>
#include <eina/settings2.h>
#include <eina/artwork2.h>

// Redefine some types and enums
#define EinaPlagin GelPlugin

#define EINA_PLAGIN_SERIAL GEL_PLUGIN_SERIAL
#define EINA_PLAGIN_GENERIC_AUTHOR "xuzo <xuzo@cuarentaydos.com>"
#define EINA_PLAGIN_GENERIC_URL    "http://eina.sourceforge.net/"
#define EINA_PLAGIN(p)      GEL_PLUGIN(p)

#ifdef EINA_PLAGIN_DATA_TYPE
#define EINA_PLAGIN_DATA(p) ((EINA_PLAGIN_DATA_TYPE *) EINA_PLAGIN(p)->data)
#endif


// --
// Access to internal values
// --
#define eina_plagin_get_app(p) \
	gel_plugin_get_app(p)
/*
LomoPlayer*
eina_plagin_get_lomo(EinaPlagin *self);
*/

// --
// Utility functions
// --
gchar*
eina_plagin_build_resource_path(EinaPlagin *plugin, gchar *resource);
gchar*
eina_plagin_build_userdir_path (EinaPlagin *plugin, gchar *path);

#define eina_plagin_verbose(...) _gel_debug(GEL_DEBUG_LEVEL_VERBOSE, __VA_ARGS__)
#define eina_plagin_debug(...)   _gel_debug(GEL_DEBUG_LEVEL_DEBUG,   __VA_ARGS__)
#define eina_plagin_info(...)    _gel_debug(GEL_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#define eina_plagin_warn(...)    _gel_debug(GEL_DEBUG_LEVEL_WARN,    __VA_ARGS__)
#define eina_plagin_error(...)   _gel_debug(GEL_DEBUG_LEVEL_ERROR,   __VA_ARGS__)

// --
// Dock handling (dock is managed by EinaPlagin currently, but there are plans
// to use EinaPlagin in the future, so use eina_plagin_dock_* macros if you
// are writting a plugin)
// --
gboolean
eina_plagin_add_dock_widget(EinaPlagin *self, gchar *id, GtkWidget *label, GtkWidget *widget);

gboolean
eina_plagin_remove_dock_widget(EinaPlagin *self, gchar *id);

gboolean
eina_plagin_switch_dock(EinaPlagin *self, gchar *id);

// --
// Configuration handling
// --
gboolean eina_plagin_add_configuration_widget
(EinaPlagin *plugin, GtkImage *icon, GtkLabel *label, GtkWidget *widget);
gboolean eina_plagin_remove_configuration_widget
(EinaPlagin *plugin,  GtkWidget *widget);

// --
// Artwork handling (cover replacement)
// --
EinaArtwork*
eina_plagin_get_artwork(EinaPlagin *plugin);

void
eina_plagin_add_artwork_provider(EinaPlagin *plugin, gchar *id,
	EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel);
void
eina_plagin_remove_artwork_provider(EinaPlagin *plugin, gchar *id);

// --
// Lomo events
// --
void
eina_plagin_attach_events(EinaPlagin *plugin, ...);
void
eina_plagin_deattach_events(EinaPlagin *plugin, ...);

#endif /* _EINA_PLUGIN */
