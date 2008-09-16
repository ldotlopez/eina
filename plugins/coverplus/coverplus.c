#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#include <eina/iface.h>

static guint source_id = 0;

gboolean coverplus_dummy(gpointer data)
{
	EinaCover *cover = EINA_COVER(data);
	source_id = 0;
	eina_cover_backend_fail(cover);
	return FALSE;
}

void coverplus_search(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	source_id = g_timeout_add(5000, (GSourceFunc) coverplus_dummy, cover);
}

void coverplus_cancel(EinaCover *cover, gpointer data)
{
	if (source_id <= 0)
		return;

	g_source_remove(source_id);
	source_id = 0;
}

EINA_PLUGIN_FUNC gboolean
coverplus_exit(EinaPlugin *self)
{
	if (source_id > 0)
		g_source_remove(source_id);
	eina_plugin_free(self);
	return TRUE;
}

EINA_PLUGIN_FUNC EinaPlugin*
coverplus_init(GelHub *app, EinaIFace *iface)
{
	EinaCover *cover;
	EinaPlugin *self = eina_plugin_new(iface,
		"coverplus", "cover", NULL, coverplus_exit,
		NULL, NULL, NULL);
	
	cover = eina_iface_get_cover(iface);
	if (cover == NULL)
	{
		gel_error("Cannot get a valid cover object, aborting");
		eina_plugin_free(self);
		return NULL;
	}

	eina_cover_add_backend(cover, "coverplus", coverplus_search, coverplus_cancel, NULL);
	return self;
}

