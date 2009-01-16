/*
 * plugins/coverplus/iface.c
 *
 * Copyright (C) 2004-2009 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#define EINA_PLUGIN_NAME "coverplus"
#define EINA_PLUGIN_DATA_TYPE CoverPlus

#include <config.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <eina/eina-plugin.h>
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
coverplus_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	EinaArtwork *artwork = eina_plugin_get_artwork(plugin);
	if (!artwork)
	{
		g_set_error_literal(error, coverplus_quark(), NO_ARTWORK, "No artwork object available");
		return FALSE;
	}

	CoverPlus *self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);

	self->infolder = coverplus_infolder_new(plugin, error);
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
		gel_warn("Cannot init Infolder Coverplus add-on: %s", (*error)->message);
		return FALSE;
	}

	eina_artwork_add_provider(artwork, "coverplus-banshee",
		coverplus_banshee_search_cb, NULL,
		NULL); 

	plugin->data = self;

	return TRUE;
}

gboolean
coverplus_exit(GelApp *app, EinaPlugin *plugin, GError **error)
{
	eina_plugin_remove_artwork_provider(plugin, "coverplus-infolder");
	eina_plugin_remove_artwork_provider(plugin, "coverplus-banshee");
	g_free(EINA_PLUGIN_DATA(plugin));

	return TRUE;
}

G_MODULE_EXPORT EinaPlugin coverplus_plugin = {
	EINA_PLUGIN_SERIAL,
	"coverplus", PACKAGE_VERSION,
	N_("Enhace your covers"),
	N_("Brings Eina several simple but fundamental cover providers like:\n"
	   "· In-folder cover discover\n"
	   "· Banshee (on Linux) covers"),
	NULL,
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL, 

	coverplus_init, coverplus_exit,

	NULL, NULL
};

