#define GEL_DOMAIN "Eina::Log"

#include <config.h>
#include <gmodule.h>
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <lomo/player.h>
#include <eina/lomo.h>

#ifdef PSTR
#undef PSTR
#endif

#define PSTR(p) gel_plugin_stringify(p)

static void
lomo_connect(LomoPlayer *lomo);

static void
plugin_load_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Load plugin '%s'", PSTR(plugin));
}

static void
plugin_unload_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Unload plugin '%s'", PSTR(plugin));
}

static void
plugin_init_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	LomoPlayer *lomo;
	gel_debug("Init plugin '%s'", PSTR(plugin));

	if (g_str_equal(plugin->name, "lomo") && ((lomo = GEL_APP_GET_LOMO(app)) != NULL))
		lomo_connect(lomo);
}

static void
plugin_fini_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Fini plugin '%s'", PSTR(plugin));
}

static void
lomo_signal_cb(gchar *signal)
{
	gel_debug("Lomo signal: %s", signal);
}

static void
lomo_error_cb(LomoPlayer *lomo, GError *error, gpointer data)
{
	gel_debug("Lomo error: %s", error->message);
}

static void
lomo_connect(LomoPlayer *lomo)
{
	const gchar *lomo_signals[] =
	{
		"play",
		"pause",
		"stop",
		"seek",
		"mute",
		// "add",
		// "del",
		"change",
		"clear",
		"repeat",
		"random",
		"eos",
		"error",
		// "tag",
		// "all_tags"
	};
	gint i;
	
	for (i = 0; i < G_N_ELEMENTS(lomo_signals); i++)
		g_signal_connect_swapped(lomo, lomo_signals[i], (GCallback) lomo_signal_cb, (gpointer) lomo_signals[i]);

	g_signal_connect(lomo, "error", (GCallback) lomo_error_cb, NULL);
}

static gboolean
log_init(GelPlugin *plugin, GError **error)
{
	GelApp *app = gel_plugin_get_app(plugin);

	LomoPlayer *lomo;
	if ((lomo = GEL_APP_GET_LOMO(app)) != NULL)
		lomo_connect(lomo);

	g_signal_connect(app, "plugin-load",   (GCallback) plugin_load_cb, NULL);
	g_signal_connect(app, "plugin-unload", (GCallback) plugin_unload_cb, NULL);
	g_signal_connect(app, "plugin-init",   (GCallback) plugin_init_cb, NULL);
	g_signal_connect(app, "plugin-fini",   (GCallback) plugin_fini_cb, NULL);

	return TRUE;
}
static gboolean
log_fini(GelPlugin *plugin, GError **error)
{
	return TRUE;
}

G_MODULE_EXPORT GelPlugin log_plugin = {
	GEL_PLUGIN_SERIAL,
	"log", PACKAGE_VERSION,
	N_("Build-in log"), NULL,
	NULL, NULL, NULL,
	log_init, log_fini,
	NULL, NULL
};

