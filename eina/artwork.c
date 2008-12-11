#define GEL_DOMAIN "Eina::ArtworkSingleton"

#include <eina/base.h>
#include <eina/lomo.h>
#include <eina/artwork.h>

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, gpointer data);

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

	eina_artwork_set_stream(obj, (LomoStream*) lomo_player_get_current_stream(GEL_HUB_GET_LOMO(hub)));
	g_signal_connect(GEL_HUB_GET_LOMO(hub), "change",
		(GCallback) lomo_change_cb, obj);
	return TRUE;
}

G_MODULE_EXPORT gboolean artwork_exit
(gpointer data)
{
	EinaArtwork *obj = EINA_ARTWORK(data);
	g_object_unref(obj);
	return TRUE;
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, gpointer data)
{
	eina_artwork_set_stream(EINA_ARTWORK(data), (LomoStream *) lomo_player_get_nth(lomo, to));
}

G_MODULE_EXPORT GelHubSlave artwork_connector = {
	"artwork",
	&artwork_init,
	&artwork_exit
};

