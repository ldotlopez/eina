#ifndef _EINA_IFACE
#define _EINA_IFACE

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <lomo/player.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>
#include <eina/player.h>
#include <eina/fs.h>
#include <eina/eina-cover.h>

#define EINA_PLUGIN_SERIAL 1

typedef struct _EinaIFace EinaIFace;

typedef struct _EinaPluginPrivate EinaPluginPrivate;
typedef struct EinaPlugin {
	guint serial;            // EINA_PLUGIN_SERIAL
	const gchar *name;       // "My cool plugin"
	const gchar *version;    // "1.0.0"
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

gboolean
eina_plugin_is_enabled(EinaPlugin *plugin);

// --
// Handling plugins by the EinaIFace
// --
GList *
eina_iface_list_available_plugins(EinaIFace *iface);

GList *
eina_iface_list_available_plugins(EinaIFace *self);

EinaPlugin*
eina_iface_query_plugin_by_name(EinaIFace *iface, gchar *plugin_name);
EinaPlugin*
eina_iface_query_plugin_by_path(EinaIFace *iface, gchar *plugin_path);

GList *
eina_iface_list_available_plugins(EinaIFace *self);

EinaPlugin*
eina_iface_load_plugin_by_name(EinaIFace *self, gchar* plugin_name);
EinaPlugin*
eina_iface_load_plugin_by_path(EinaIFace *self, gchar *plugin_name, gchar *plugin_path);
void
eina_iface_unload_plugin(EinaIFace *iface, EinaPlugin *plugin);

gboolean
eina_iface_init_plugin(EinaIFace *self, EinaPlugin *plugin);
gboolean
eina_iface_fini_plugin(EinaIFace *self, EinaPlugin *plugin);

EinaPlayer *
eina_iface_get_player(EinaIFace *self);
GtkWindow *
eina_iface_get_main_window(EinaIFace *self);

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
// Configuration handling
// --
gboolean eina_plugin_add_configuration_widget
(EinaPlugin *plugin, GtkImage *icon, GtkLabel *label, GtkWidget *widget);
gboolean eina_plugin_remove_configuration_widget
(EinaPlugin *plugin,  GtkWidget *widget);

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

