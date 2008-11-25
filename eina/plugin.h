#ifndef _EINA_PLUGIN
#define _EINA_PLUGIN

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <lomo/player.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>
#include <eina/player.h>
#include <eina/fs.h>
#include <eina/settings.h>
#include <eina/eina-cover.h>

#define EINA_PLUGIN_SERIAL 2
#define EINA_PLUGIN(p)     ((EinaPlugin *) p)

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
// New / free
// --
EinaPlugin*
eina_plugin_new(GelHub *hub, gchar *plugin_path, gchar *symbol);
gboolean
eina_plugin_free(EinaPlugin *self);

// --
// Init / fini
// --
gboolean
eina_plugin_init(EinaPlugin *self);
gboolean
eina_plugin_fini(EinaPlugin *self);

// --
// Access to internal values
// --
const gchar*
eina_plugin_get_pathname(EinaPlugin *self);
gboolean
eina_plugin_is_enabled(EinaPlugin *self);
GelHub*
eina_plugin_get_hub(EinaPlugin *self);
LomoPlayer*
eina_plugin_get_lomo(EinaPlugin *self);

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


// Access to plugin's data if defined
#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) ((EinaPlugin *) p)->data)
#endif

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

#endif /* _EINA_IFACE */
