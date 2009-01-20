/*
 * plugins/recently2/iface.c
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

#include <config.h>

#define GEL_DOMAIN "Eina::Plugin::Recently"
#define EINA_PLUGIN_DATA_TYPE "Recently"
#define EINA_PLUGIN_NAME "Recently"

#include <eina/eina-plugin.h>
#include "plugins/adb/adb.h"
#include "recently.h"

gboolean
recently_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	// Load adb
	GError *err = NULL;
	if (!gel_app_load_plugin_by_name(app, "adb", &err))
	{
		g_set_error(error, recently_quark(), RECENTLY_CANNOT_LOAD_ADB, 
			N_("Cannot load adb plugin: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}

	Recently *self = recently_new(app, error);
	if (self == NULL)
		return FALSE;

	plugin->data = self;
	return TRUE;
}

gboolean
recently_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	GError *err = NULL;
	if (!gel_app_unload_plugin_by_name(app, "adb", &err))
	{
		g_set_error(error, recently_quark(), RECENTLY_CANNOT_UNLOAD_ADB, 
			N_("Cannot unload adb plugin: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}

	return TRUE;
}

EinaPlugin recently2_plugin = {
	EINA_PLUGIN_SERIAL,
	"recently", PACKAGE_VERSION,
	N_("Stores your recent playlists"),
	N_("Recently plugin 2. ADB based version"), // long desc
	NULL, // icon
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	recently_plugin_init, recently_plugin_fini,
	NULL, NULL
};
