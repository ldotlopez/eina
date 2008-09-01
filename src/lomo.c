#define GEL_DOMAIN "Eina::Lomo"
#include <gmodule.h>
#include <liblomo/player.h>
#include <gel/gel.h>
#include "lomo.h"

/*
 * Init and fini functions
 */
G_MODULE_EXPORT gboolean eina_lomo_init
(GHub *hub, gint *argc, gchar ***argv)
{
	GError   *err = NULL;
	LomoPlayer *engine = NULL;

	lomo_init();
	engine = lomo_player_new_from_params("gconfaudiosink name=sink", &err);

	if (!engine)
	{
		gel_error("Cannot create LomoPlayer engine: %s", err->message);
		g_error_free(err);
		return FALSE;
	}

    if (!g_hub_shared_set(hub, "lomo", engine))
	{
		gel_error("Cannot allocate engine into GHub");
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
 * GHub Slave
 */
G_MODULE_EXPORT GHubSlave lomo_connector =
{
	"lomo",
	&eina_lomo_init,
	&eina_lomo_fini
};

