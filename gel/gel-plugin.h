#ifndef _GEL_PLUGIN
#define _GEL_PLUGIN

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GelPlugin        GelPlugin;
#include <gel/gel-app.h>

#define GEL_PLUGIN(p)     ((GelPlugin *) p)
#define GEL_PLUGIN_SERIAL 0

typedef struct _GelPluginPrivate GelPluginPrivate;
struct _GelPlugin {
	guint serial;            // GEL_PLUGIN_SERIAL
	const gchar *name;       // "My cool plugin"
	const gchar *version;    // "1.0.0"
	const gchar *short_desc; // "This plugins makes my app cooler"
	const gchar *long_desc;  // "Blah blah blah..."
	const gchar *icon;       // "icon.png", relative path
	const gchar *author;     // "me <me@company.com>"
	const gchar *url;        // "http://www.company.com"
	const gchar *depends;    // "foo,bar,baz" or NULL

	gboolean (*init) (struct _GelPlugin *plugin, GError **error); // Init function
	gboolean (*fini) (struct _GelPlugin *plugin, GError **error); // Exit function

	gpointer data; // Plugin's own data

	GelPluginPrivate *priv;
};

enum {
	GEL_PLUGIN_NO_ERROR = 0,
	GEL_PLUGIN_DYNAMIC_LOADING_NOT_SUPPORTED,
	GEL_PLUGIN_SYMBOL_NOT_FOUND,
	GEL_PLUGIN_INVALID_SERIAL
};

GelPlugin* gel_plugin_new(GelApp *app, gchar *pathname, gchar *symbol, GError **error);
#ifdef GEL_COMPILATION
void gel_plugin_ref  (GelPlugin *plugin);
void gel_plugin_unref(GelPlugin *plugin);

gboolean gel_plugin_init(GelPlugin *plugin, GError **error);
gboolean gel_plugin_fini(GelPlugin *plugin, GError **error);
#endif

gboolean     gel_plugin_matches     (GelPlugin *plugin, gchar *pathname, gchar *symbol);
GelApp*      gel_plugin_get_app     (GelPlugin *plugin);
// const gchar* gel_plugin_get_pathname(GelPlugin *plugin);
const gchar* gel_plugin_stringify   (GelPlugin *plugin);
gboolean     gel_plugin_is_enabled  (GelPlugin *plugin);

G_END_DECLS

#endif /* _GEL_PLUGIN */
