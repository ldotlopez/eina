/*
 * plugins/adb/adb.c
 *
 * Copyright (C) 2004-2010 Eina
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
#include "eina-adb.h"
#include "register.h"

GEL_DEFINE_QUARK_FUNC(adb)

static gboolean
lomo_insert_hook(LomoPlayer *lomo, LomoPlayerHookEvent event, gpointer ret, EinaAdb *adb)
{
	if (event.type != LOMO_PLAYER_HOOK_INSERT)
		return FALSE;
	g_return_val_if_fail(EINA_IS_ADB(adb), FALSE);
	g_return_val_if_fail(LOMO_IS_STREAM(event.stream), FALSE);

	eina_adb_lomo_stream_set_sid(adb, event.stream);

	return FALSE;
}

static gboolean
adb_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
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
		g_set_error(error, adb_quark(), 1, N_("Error setting database '%s'"), db_path);
		g_free(db_path);
		g_object_unref(adb);
		return FALSE;
	}
	g_free(db_path);

	// Register into App
	if (!gel_app_shared_set(app, "adb", adb))
	{
		g_set_error(error, adb_quark(), 1,
			N_("Cannot register ADB object into GelApp"));
		g_object_unref(adb);
		return FALSE;
	}

	LomoPlayer *lomo = eina_plugin_get_lomo(plugin);
	
	GList *pl = lomo_player_get_playlist(lomo);
	GList *iter = pl;
	while (iter)
	{
		eina_adb_lomo_stream_set_sid(adb, LOMO_STREAM(iter->data));
		iter = iter->next;
	}
	g_list_free(pl);

	lomo_player_hook_add(lomo, (LomoPlayerHook) lomo_insert_hook, adb);

	adb_register_start(adb, lomo);

	return ret;
}

static gboolean
adb_plugin_exit(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = eina_plugin_get_lomo(plugin);
	EinaAdb *adb = gel_app_shared_get(app, "adb");
	adb_register_stop(adb, lomo);

	g_object_unref(adb);

	return TRUE;
}

EINA_PLUGIN_SPEC(adb,
	PACKAGE_VERSION, "lomo",
	NULL, NULL,

	N_("Audio database"), NULL, "adb.png",

	adb_plugin_init, adb_plugin_exit
);

