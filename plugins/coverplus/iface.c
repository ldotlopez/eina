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

// Hold sub-plugins data and backends
typedef struct CoverPlus {
	CoverPlusInfolder *infolder;
	ArtBackend *infolder_backend;
	ArtBackend *banshee_backend;
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
	// Load artwork module, and get the object since EinaPlugin has no support
	// ATM
	if (!gel_app_load_plugin_by_name(app, "art", error))
		return FALSE;

	Art *art = GEL_APP_GET_ART(app);
	if (!art)
		return FALSE;

	CoverPlus *self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);

	// Add infolder search
	self->infolder = coverplus_infolder_new(plugin, error);
	if (self->infolder)
		self->infolder_backend = art_add_backend(art, "coverplus-infolder",
			(ArtFunc) coverplus_infolder_art_search_cb, NULL,
			self->infolder);
	else
		gel_warn("Cannot init Infolder Coverplus add-on: %s", (*error)->message);

	// Add banshee search
	self->banshee_backend = art_add_backend(art, "coverplus-banshee",
		(ArtFunc) coverplus_banshee_art_search_cb, NULL,
		NULL);

	plugin->data = self;
	return TRUE;
}

gboolean
coverplus_exit(GelApp *app, EinaPlugin *plugin, GError **error)
{
	CoverPlus *self = EINA_PLUGIN_DATA(plugin);
	if (!self)
		return TRUE;

	Art *art = GEL_APP_GET_ART(app);
	if (!art)
		return FALSE;

	if (self->infolder_backend)
	{
		art_remove_backend(art, self->infolder_backend);
		self->infolder_backend = NULL;
	}
	if (self->banshee_backend)
	{
		art_remove_backend(art, self->banshee_backend);
		self->banshee_backend = NULL;
	}
	g_free(self);
	plugin->data = NULL;

	if (!gel_app_unload_plugin_by_name(app, "art", error))
		return FALSE;

	return TRUE;
}

G_MODULE_EXPORT EinaPlugin coverplus_plugin = {
	EINA_PLUGIN_SERIAL,
	"coverplus", PACKAGE_VERSION, NULL,
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,

	N_("Enhace your covers"),
	N_("Brings Eina several simple but fundamental cover providers like:\n"
	   "· In-folder cover discover\n"
	   "· Banshee (on Linux) covers"),
	NULL,

	coverplus_init, coverplus_exit,

	NULL, NULL, NULL
};

