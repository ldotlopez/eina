#define GEL_DOMAIN "Eina::ArtworkSingleton"

#include <eina/base.h>
#include <eina/lomo.h>
#include <eina/artwork.h>
#include <eina/plugin.h>

G_MODULE_EXPORT gboolean artwork_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaArtwork *obj = eina_artwork_new();

	if (!gel_hub_shared_set(hub, "artwork", obj))
	{
		gel_error("Cannot create singleton for EinaArtwork object");
		g_object_unref(obj);
		return FALSE;
	}

	return TRUE;
}

G_MODULE_EXPORT gboolean artwork_exit
(gpointer data)
{
	/*
	EinaArtwork *obj = EINA_ARTWORK(data);
	g_object_unref(obj);
	*/
	return TRUE;
}

G_MODULE_EXPORT GelHubSlave artwork_connector = {
	"artwork",
	&artwork_init,
	&artwork_exit
};
