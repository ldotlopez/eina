#define GEL_DOMAIN "Eina::Lomo"
#include <gmodule.h>
#include <lomo/player.h>
#include <gel/gel.h>

/*
 * Init and fini functions
 */
G_MODULE_EXPORT gboolean eina_lomo_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	GError   *err = NULL;
	LomoPlayer *engine = NULL;

	engine = lomo_player_new_with_opts("audio-output", "autoaudiosink", NULL);

	if (!engine)
	{
		gel_error("Cannot create LomoPlayer engine: %s", err->message);
		g_error_free(err);
		return FALSE;
	}

    if (!gel_hub_shared_set(hub, "lomo", engine))
	{
		gel_error("Cannot allocate engine into GelHub");
		g_object_unref(engine);
        return FALSE;
    }

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_lomo_fini
(gpointer data)
{
	g_object_unref((LomoPlayer *) data);
	return TRUE;
}

/*
 * GelHub Slave
 */
G_MODULE_EXPORT GelHubSlave lomo_connector =
{
	"lomo",
	&eina_lomo_init,
	&eina_lomo_fini
};

