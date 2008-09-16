#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#include <eina/iface.h>


// Timeout-based cover backend. For testing
static guint coverplus_timeout_test_source_id = 0;
void coverplus_timeout_test_search(EinaCover *cover, const LomoStream *stream, gpointer data);
void coverplus_timeout_test_cancel(EinaCover *cover, gpointer data);
static gboolean coverplus_timeout_test_result(gpointer data);


// Timeout-test
void
coverplus_timeout_test_search(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	 coverplus_timeout_test_source_id = g_timeout_add(5000, (GSourceFunc) coverplus_timeout_test_result, cover);
}

void
coverplus_timeout_test_cancel(EinaCover *cover, gpointer data)
{
	if (coverplus_timeout_test_source_id <= 0)
		return;

	g_source_remove(coverplus_timeout_test_source_id);
	coverplus_timeout_test_source_id = 0;
}

static gboolean
coverplus_timeout_test_result(gpointer data)
{
	EinaCover *cover = EINA_COVER(data);
	coverplus_timeout_test_source_id = 0;
	eina_cover_backend_fail(cover);
	return FALSE;
}

EINA_PLUGIN_FUNC gboolean
coverplus_exit(EinaPlugin *self)
{
	if (coverplus_timeout_test_source_id > 0)
		g_source_remove(coverplus_timeout_test_source_id);
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

	eina_cover_add_backend(cover, "coverplus-timeout-test",
		coverplus_timeout_test_search, coverplus_timeout_test_cancel, NULL);
	return self;
}

