/*
 * eina/adb/eina-adb-plugin.c
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

#include "eina-adb-plugin.h"
#include "register.h"
#include <eina/lomo/eina-lomo-plugin.h>

GEL_DEFINE_QUARK_FUNC(eina_adb_plugin)
EINA_DEFINE_EXTENSION(EinaAdbPlugin, eina_adb_plugin, EINA_TYPE_ADB_PLUGIN)

static gboolean
eina_adb_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaAdb *adb = eina_adb_new();

	const gchar *conf_dir = g_get_user_config_dir();
	if (!conf_dir)
		conf_dir = ".cache";

	gchar *db_path = g_build_filename(conf_dir, PACKAGE, "adb.db", NULL);
	gchar *db_dirname = g_path_get_dirname(db_path);
	g_mkdir_with_parents(db_dirname, 0755);
	g_free(db_dirname);

	gboolean ret;
	if (!(ret = eina_adb_set_db_file(adb, db_path)))
	{
		g_set_error(error, eina_adb_plugin_quark(),
			EINA_ADB_PLUGIN_ERROR_UNABLE_TO_SET_DB_FILE, N_("Error setting database '%s'"), db_path);
		g_free(db_path);
		g_object_unref(adb);
		return FALSE;
	}
	g_free(db_path);

	// Register into App
	if (!eina_application_set_interface(app, "adb", adb))
	{
		g_set_error(error, eina_adb_plugin_quark(), 
		            EINA_ADB_PLUGIN_ERROR_CANNOT_REGISTER_INTERFACE, N_("Cannot register ADB interface"));
		g_object_unref(adb);
		return FALSE;
	}

	adb_register_start(adb, eina_application_get_interface(app, "lomo"));

	return ret;
}

static gboolean
eina_adb_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaAdb    *adb  = eina_application_get_adb (app);
	LomoPlayer *lomo = eina_application_get_lomo(app);

	adb_register_stop(adb, lomo);
	g_object_unref(adb);

	return TRUE;
}

/**
 * eina_application_get_adb:
 * @application: An #EinaApplication
 *
 * Get the #EinaAdb object from @application
 *
 * Returns: (transfer none): The #EinaAdb
 */
EinaAdb*
eina_application_get_adb(EinaApplication *application)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(application), NULL);
	return eina_application_get_interface(application, "adb");
}

