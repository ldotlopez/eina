#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#define EINA_PLUGIN_NAME "coverplus"
#define EINA_PLUGIN_DATA_TYPE CoverPlus

#include <config.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <eina/plugin.h>
#include "infolder.h"
#include "banshee.h"

// Hold sub-plugins data
typedef struct CoverPlus {
	CoverPlusInfolder *infolder;
} CoverPlus;

GQuark
coverplus_quark(void)
{
	static guint ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("Eina-Plugin-CoverPlus");
	return ret;
}

enum {
	NO_ARTWORK = 1
};

// --
// Main
// --
gboolean
coverplus_init(EinaPlugin *plugin, GError **error)
{
	EinaArtwork *artwork = eina_plugin_get_artwork(plugin);
	if (!artwork)
	{
		g_set_error_literal(error, coverplus_quark(), NO_ARTWORK, "No artwork object available");
		return FALSE;
	}

	CoverPlus *self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);

	// Initialize sub-plugins
	GError *err = NULL;

	self->infolder = coverplus_infolder_new(plugin, &err);
	if (self->infolder)
	{
		// Add providers
		eina_artwork_add_provider(artwork, "coverplus-infolder",
			(EinaArtworkProviderSearchFunc) coverplus_infolder_search_cb,
			(EinaArtworkProviderCancelFunc) coverplus_infolder_cancel_cb,
			self->infolder);
	}
	else
	{
		gel_warn("Cannot init Infolder Coverplus add-on: %s", err ? err->message : "no reason");
		if (err)
			g_error_free(err);
	}

	eina_artwork_add_provider(artwork, "coverplus-banshee",
		coverplus_banshee_search_cb, NULL,
		NULL); 

	plugin->data = self;

	return TRUE;
}

gboolean
coverplus_exit(EinaPlugin *plugin, GError **error)
{
	eina_plugin_remove_artwork_provider(plugin, "coverplus-infolder");
	// eina_plugin_remove_artwork_provider(plugin, "coverplus-banshee");
	g_free(EINA_PLUGIN_DATA(plugin));

	return TRUE;
}

G_MODULE_EXPORT EinaPlugin coverplus_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("Cover plus"),
	"0.7.0",
	N_("Enhace your covers"),
	N_("Brings Eina several simple but fundamental cover providers like:\n"
	   "· In-folder cover discover\n"
	   "· Banshee (on Linux) covers"),
	NULL,
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net/",

	coverplus_init, coverplus_exit,

	NULL, NULL
};

