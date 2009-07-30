/*
 * plugins/adb/iface.c
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

#define GEL_DOMAIN "Eina::Plugin::ADB"
#define EINA_PLUGIN_DATA_TYPE Adb

#include <config.h>
#include <eina/eina-plugin.h>
#include <plugins/adb/adb.h>

static gboolean
adb_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	Adb *self;

	// Create Adb interface
	if ((self = adb_new(app, error)) == NULL)
	{
		g_set_error(error, adb_quark(), ADB_CANNOT_GET_LOMO, N_("Cannot access lomo"));
		return FALSE;
	}
	plugin->data = (gpointer) self;

	// Register into App
	if (!gel_app_shared_set(app, "adb", self))
	{
		g_set_error(error, adb_quark(), ADB_CANNOT_REGISTER_OBJECT,
			N_("Cannot register ADB object into GelApp"));
		adb_free(self);
		return FALSE;
	}

	return TRUE;
}

static gboolean
adb_plugin_exit(GelApp *app, EinaPlugin *plugin, GError **error)
{
	Adb *self = EINA_PLUGIN_DATA(plugin);

	adb_free(self);
	gel_app_shared_unregister(app, "adb");
	return TRUE;
}

EINA_PLUGIN_SPEC(adb,
	PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Audio database"), NULL, NULL,

	adb_plugin_init, adb_plugin_exit
);

