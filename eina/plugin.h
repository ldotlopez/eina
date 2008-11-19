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
eina_plugin_new(GelHub *hub, gchar *plugin_name, gchar *plugin_path);
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
// Utility macros
// --

// Access to plugin's data if defined
#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) ((EinaPlugin *) p)->data)
#endif

#endif // _EINA_PLUGIN
