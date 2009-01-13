#define GEL_DOMAIN "Eina::ArtworkSingleton"

#include <config.h>
#include <gel/gel.h>
#include <eina/artwork.h>

enum {
	ARTWORK_NO_ERROR = 0,
	ARTWORK_ERROR_CANNOT_REGISTER_OBJECT,
	ARTWORK_ERROR_CANNOT_UNREGISTER_OBJECT
};

static GQuark
artwork_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("artwork");
	return ret;
}

static gboolean
artwork_init (GelPlugin *plugin, GError **error)
{
	GelApp *app = gel_plugin_get_app(plugin);
	EinaArtwork *obj = eina_artwork_new();
	g_object_ref(obj);

	if (!gel_app_shared_set(app, "artwork", obj))
	{
		g_set_error(error, artwork_quark(), ARTWORK_ERROR_CANNOT_REGISTER_OBJECT,
			N_("Cannot create singleton for EinaArtwork object"));
		g_object_unref(obj);
		return FALSE;
	}

	return TRUE;
}

static gboolean
artwork_fini(GelPlugin *plugin, GError **error)
{
	GelApp *app = gel_plugin_get_app(plugin);
	EinaArtwork *obj = gel_app_shared_get(app, "artwork");
	if (!obj)
	{
		g_set_error(error, artwork_quark(), ARTWORK_ERROR_CANNOT_UNREGISTER_OBJECT,
			N_("Cannot fetch singleton for EinaArtwork object"));
		return FALSE;
	}
	gel_app_shared_unregister(app, "artwork");
	g_object_unref(obj);
	return TRUE;
}

G_MODULE_EXPORT GelPlugin artwork_plugin = {
	GEL_PLUGIN_SERIAL,
	"artwork", PACKAGE_VERSION,
	N_("Build-in artwork plugin"), NULL,
	NULL, NULL, NULL,
	artwork_init, artwork_fini,
	NULL, NULL
};

