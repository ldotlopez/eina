#define GEL_DOMAIN "Eina::Log"

#include <gmodule.h>
#include <gel/gel.h>
#include "log.h"

void on_eina_logel_hub_module_load  (GelHub *hub, const gchar *name, gpointer data);
void on_eina_logel_hub_module_unload(GelHub *hub, const gchar *name, gpointer data);
void on_eina_logel_hub_module_ref   (GelHub *hub, const gchar *name, guint refs, gpointer data);
void on_eina_logel_hub_module_unref (GelHub *hub, const gchar *name, guint refs, gpointer data);

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean
eina_log_init (GelHub *hub, gint *argc, gchar ***argv)
{
	g_signal_connect(G_OBJECT(hub), "module-load",
		G_CALLBACK(on_eina_logel_hub_module_load), NULL);

	g_signal_connect(G_OBJECT(hub), "module-unload",
		G_CALLBACK(on_eina_logel_hub_module_unload), NULL);

	g_signal_connect(G_OBJECT(hub), "module-unref",
		G_CALLBACK(on_eina_logel_hub_module_ref), NULL);

	g_signal_connect(G_OBJECT(hub), "module-unref",
		G_CALLBACK(on_eina_logel_hub_module_unref), NULL);
	
	return TRUE;
}

void
on_eina_logel_hub_module_load(GelHub *hub, const gchar *name, gpointer data)
{
	gel_info("Loaded module '%s'", name);
}

void
on_eina_logel_hub_module_unload(GelHub *hub, const gchar *name, gpointer data)
{
	gel_info("Unloaded module '%s'", name);
}

void
on_eina_logel_hub_module_ref(GelHub *hub, const gchar *name, guint refs, gpointer data)
{
	gel_info("Referenced module '%s': %d", name, refs);
}

void
on_eina_logel_hub_module_unref(GelHub *hub, const gchar *name, guint refs, gpointer data)
{
	gel_info("Unreferenced module '%s': %d", name, refs);
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

