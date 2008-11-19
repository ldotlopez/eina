#define GEL_DOMAIN "Eina::Log"

#include <gmodule.h>
#include <lomo/player.h>
#include <gel/gel.h>

static void
on_lomo_error       (LomoPlayer *lomo, GError *err, gchar *message);
static void
on_hub_module_load  (GelHub *hub, const gchar *name, gpointer data);
static void
on_hub_module_unload(GelHub *hub, const gchar *name, gpointer data);
static void
on_hub_module_ref   (GelHub *hub, const gchar *name, guint refs, gpointer data);
static void
on_hub_module_unref (GelHub *hub, const gchar *name, guint refs, gpointer data);
static void
on_lomo_signal(gchar *signal);

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean
eina_log_init (GelHub *hub, gint *argc, gchar ***argv)
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

	g_signal_connect(gel_hub_shared_get(hub, "lomo"), "error",
		G_CALLBACK(on_lomo_error), NULL);

	g_signal_connect(G_OBJECT(hub), "module-load",
		G_CALLBACK(on_hub_module_load), NULL);

	g_signal_connect(G_OBJECT(hub), "module-unload",
		G_CALLBACK(on_hub_module_unload), NULL);

	g_signal_connect(G_OBJECT(hub), "module-unref",
		G_CALLBACK(on_hub_module_ref), NULL);

	g_signal_connect(G_OBJECT(hub), "module-unref",
		G_CALLBACK(on_hub_module_unref), NULL);
	
	for (i = 0; i < G_N_ELEMENTS(lomo_signals); i++)
		g_signal_connect_swapped(gel_hub_shared_get(hub, "lomo"), lomo_signals[i],
		G_CALLBACK(on_lomo_signal), (gpointer) lomo_signals[i]);
	return TRUE;
}

void on_lomo_error(LomoPlayer *lomo, GError *err, gchar *message)
{
	gel_error("Got lomo error (%s): '%s'", message, err->message);
}

void
on_hub_module_load(GelHub *hub, const gchar *name, gpointer data)
{
	gel_debug("Loaded module '%s'", name);
}

void
on_hub_module_unload(GelHub *hub, const gchar *name, gpointer data)
{
	gel_debug("Unloaded module '%s'", name);
}

void
on_hub_module_ref(GelHub *hub, const gchar *name, guint refs, gpointer data)
{
	gel_debug("Referenced module '%s': %d", name, refs);
}

void
on_hub_module_unref(GelHub *hub, const gchar *name, guint refs, gpointer data)
{
	gel_debug("Unreferenced module '%s': %d", name, refs);
}

void on_lomo_signal(gchar *signal)
{
	gel_debug("Lomo signal: %s", (gchar *) signal);
}
/* * * * * * * * * * * * * * * * * * */
/* Create the connector for the hub  */
/* * * * * * * * * * * * * * * * * * */
G_MODULE_EXPORT GelHubSlave
log_connector =
{
	"log",
	&eina_log_init,
	NULL
};

