#ifndef _EINA_IFACE
#define _EINA_IFACE

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <lomo/player.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>
#include <eina/fs.h>
#include <eina/eina-cover.h>

typedef struct _EinaIFace EinaIFace;

typedef struct _EinaPluginPrivate EinaPluginPrivate;
typedef struct EinaPlugin {
	const gchar *name;       // "My cool plugin"
	const gchar *short_desc; // "This plugins makes Eina cooler
	const gchar *long_desc;  // "Blah blah blah..."
	const gchar *icon;       // "icon.png", relative path
	const gchar *author;     // "xuzo <xuzo@cuarentaydos.com>"
	const gchar *url;        // "http://eina.sourceforge.net"

	gboolean  (*init)(struct EinaPlugin *self, GError **error); // Init function
	gboolean  (*fini)(struct EinaPlugin *self, GError **error); // Exit function

	gpointer data; // Plugin's own data

	EinaPluginPrivate *priv;
} EinaPlugin;

// --
// Access to plugin's data
// --
#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) ((EinaPlugin *) p)->data)
#endif

#if 0
/*
 * Functions to load/unload a complete plugin using EinaPlugin struct
 */
gboolean
eina_iface_plugin_register(EinaIFace *self, const EinaPlugin plugin);

gboolean
eina_iface_plugin_unregister(EinaIFace *self, gchar plugin_name);

gchar *
eina_iface_build_plugin_filename(gchar *plugin_name, gchar *filename);

gchar *
eina_iface_get_plugin_dir(gchar *plugin_name);

gchar *
eina_iface_plugin_resource_get_pathname(EinaPlugin *plugin, gchar *resource);
#endif

/*
 * Advanced functions for accesing some internals
 */

// --
// Direct access to other resources, use macros were possible
// --
#define EINA_PLUGIN(p) ((EinaPlugin *) p)

EinaIFace*
eina_plugin_get_iface(EinaPlugin *self);
#define EINA_PLUGIN_IFACE(p) eina_plugin_get_iface(p)

GelHub*
eina_iface_get_hub(EinaIFace *iface);
#define EINA_PLUGIN_HUB(p) eina_iface_get_hub(EINA_PLUGIN_IFACE(p))

LomoPlayer*
eina_iface_get_lomo(EinaIFace *iface);
#define EINA_PLUGIN_LOMO(p) eina_iface_get_lomo(EINA_PLUGIN_IFACE(p))

// --
// Dock handling (dock is managed by EinaIFace currently, but there are plans
// to use EinaPlugin in the future, so use eina_plugin_dock_* macros if you
// are writting a plugin)
// --
gboolean
eina_iface_dock_add_item(EinaIFace *iface, gchar *id, GtkWidget *label, GtkWidget *widget);
#define eina_plugin_dock_add_item(plugin,id,label,widget) \
	eina_iface_dock_add_item(EINA_PLUGIN_IFACE(plugin), id, label, widget)

gboolean
eina_iface_dock_remove_item(EinaIFace *iface, gchar *id);
#define eina_plugin_dock_remove_item(plugin,id) \
	eina_iface_dock_remove_item(EINA_PLUGIN_IFACE(plugin), id)

gboolean
eina_iface_dock_switch_item(EinaIFace *iface, gchar *id);
#define eina_plugin_dock_switch_item(plugin,id) \
	eina_iface_dock_switch_item(EINA_PLUGIN_IFACE(plugin), id)

// --
// Cover handling
// --
//
EinaCover*
eina_plugin_get_player_cover(EinaPlugin *plugin);

void
eina_plugin_cover_add_backend(EinaPlugin *plugin, gchar *id,
	EinaCoverBackendFunc search, EinaCoverBackendCancelFunc cancel);
void
eina_plugin_cover_remove_backend(EinaPlugin *plugin, gchar *id);

// --
// Lomo events
// --
void
eina_plugin_attach_events(EinaPlugin *plugin, ...);
void
eina_plugin_deattach_events(EinaPlugin *plugin, ...);

// --
// Utility functions
// --
gchar *eina_plugin_build_resource_path(EinaPlugin *plugin, gchar *resource);
gchar *eina_plugin_build_userdir_path (EinaPlugin *plugin, gchar *path);


#define eina_iface_verbose(...) _gel_debug(GEL_DEBUG_LEVEL_VERBOSE, __VA_ARGS__)
#define eina_iface_debug(...)   _gel_debug(GEL_DEBUG_LEVEL_DEBUG,   __VA_ARGS__)
#define eina_iface_info(...)    _gel_debug(GEL_DEBUG_LEVEL_INFO,    __VA_ARGS__)
#define eina_iface_warn(...)    _gel_debug(GEL_DEBUG_LEVEL_WARN,    __VA_ARGS__)
#define eina_iface_error(...)   _gel_debug(GEL_DEBUG_LEVEL_ERROR,   __VA_ARGS__)

#endif /* _EINA_IFACE */

