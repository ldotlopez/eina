/*
 * plugins/coverplus/coverplus.c
 *
 * Copyright (C) 2004-2011 Eina
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

typedef enum {
	EINA_COVERPLUS_NO_ERROR = 0,
	EINA_COVERPLUS_ERROR_NO_ARTWORK_OBJECT
} EinaCoverPlusError;

// --
// Main
// --
G_MODULE_EXPORT gboolean
coverplus_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	Art *art = GEL_APP_GET_ART(app);
	if (!art)
	{
		g_set_error(error, coverplus_quark(), EINA_COVERPLUS_ERROR_NO_ARTWORK_OBJECT,
			N_("Art object not found"));
		return FALSE;
	}

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

	gel_plugin_set_data(plugin, self);
	return TRUE;
}

G_MODULE_EXPORT gboolean
coverplus_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
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
	gel_plugin_set_data(plugin, NULL);

	return TRUE;
}

