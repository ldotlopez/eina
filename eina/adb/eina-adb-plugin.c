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

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_ADB_PLUGIN         (eina_adb_plugin_get_type ())
#define EINA_ADB_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_ADB_PLUGIN, EinaAdbPlugin))
#define EINA_ADB_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_ADB_PLUGIN, EinaAdbPlugin))
#define EINA_IS_ADB_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_ADB_PLUGIN))
#define EINA_IS_ADB_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_ADB_PLUGIN))
#define EINA_ADB_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_ADB_PLUGIN, EinaAdbPluginClass))
typedef struct {
	EinaAdb *adb;
} EinaAdbPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_ADB_PLUGIN, EinaAdbPlugin, eina_adb_plugin)

GEL_DEFINE_QUARK_FUNC(eina_adb_plugin)

static gboolean
eina_adb_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaAdbPlugin      *plugin = EINA_ADB_PLUGIN(activatable);
	EinaAdbPluginPrivate *priv = plugin->priv;

	priv->adb = eina_adb_new();

	const gchar *conf_dir = g_get_user_config_dir();
	if (!conf_dir)
		conf_dir = ".cache";

	gchar *db_path = g_build_filename(conf_dir, PACKAGE, "adb.db", NULL);
	gchar *db_dirname = g_path_get_dirname(db_path);
	g_mkdir_with_parents(db_dirname, 0755);
	g_free(db_dirname);

	gboolean ret;
	if (!(ret = eina_adb_set_db_file(priv->adb, db_path)))
	{
		g_set_error(error, eina_adb_plugin_quark(),
			EINA_ADB_PLUGIN_ERROR_UNABLE_TO_SET_DB_FILE, N_("Error setting database '%s'"), db_path);
		g_free(db_path);
		g_object_unref(priv->adb);
		priv->adb = NULL;
		return FALSE;
	}
	g_free(db_path);

	// Register into App
	if (!eina_application_set_interface(app, "adb", priv->adb))
	{
		g_set_error(error, eina_adb_plugin_quark(), 
		            EINA_ADB_PLUGIN_ERROR_CANNOT_REGISTER_INTERFACE, N_("Cannot register ADB interface"));
		g_object_unref(priv->adb);
		priv->adb = NULL;
		return FALSE;
	}

	adb_register_start(priv->adb, eina_application_get_lomo(app));

	return ret;
}

static gboolean
eina_adb_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaAdbPlugin      *plugin = EINA_ADB_PLUGIN(activatable);
	EinaAdbPluginPrivate *priv = plugin->priv;

	eina_application_set_interface(app, "adb", NULL);

	g_warn_if_fail(EINA_IS_ADB(priv->adb));
	if (priv->adb)
	{
		LomoPlayer *lomo = eina_application_get_lomo(app);
		adb_register_stop(priv->adb, lomo);

		g_object_unref(priv->adb);
		priv->adb = NULL;
	}

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

