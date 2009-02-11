/*
 * plugins/adb/adb.c
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

#define GEL_DOMAIN "Adb"
#include <config.h>
#include <glib/gi18n.h>
#include "adb.h"
#include "register.h"

// --
// Upgrade callbacks
// --
gboolean
adb_setup_0(Adb *adb, gpointer data, GError **error);

GQuark
adb_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("adb");
	return ret;
}

Adb*
adb_new(GelApp *app, GError **error)
{
	Adb *self;
	gchar   *db_path;
	sqlite3 *db = NULL;

	db_path = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "adb.db", NULL);
	if (sqlite3_open(db_path, &db) != SQLITE_OK)
	{
		gel_error("Cannot open db: %s", sqlite3_errmsg(db));
		g_free(db_path);
		return FALSE;
	}
	g_free(db_path);
	sqlite3_extended_result_codes(db, 1);

	self = g_new0(Adb, 1);
	self->db  = db;
	self->app = app;

	gpointer callbacks[] = {
		adb_setup_0,
		NULL
		};

	if (!adb_schema_upgrade(self, "core", callbacks, NULL, error))
	{
		adb_free(self);
		return NULL;
	}

	adb_register_enable(self);

	return self;
}

void
adb_free(Adb *self)
{
//	gel_app_unload_plugin_by_name(self->app, "settings");
	sqlite3_close(self->db);
	g_free(self);
}

gboolean
adb_setup_0(Adb *self, gpointer data, GError **error)
{
	gchar *q[] = {
		"DROP TABLE IF EXISTS schema_versions;",
		"CREATE TABLE IF NOT EXISTS schema_versions ("
		"schema VARCHAR(32) PRIMARY KEY,"
		"version INTERGER"
		");",

		"DROP TABLE IF EXISTS variables;",
		"CREATE TABLE IF NOT EXISTS variables ("
		"key VARCHAR(256) PRIMARY KEY,"
		"value VARCHAR(1024)"
		");",

		NULL
	};

	return adb_exec_queryes(self, q, NULL, error);
}

gchar *
adb_variable_get(Adb *self, gchar *variable)
{
	sqlite3_stmt *stmt = NULL;
	char *q = sqlite3_mprintf("SELECT value FROM variables WHERE key = '%q' LIMIT 1", variable);
	if (sqlite3_prepare_v2(self->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		sqlite3_free(q);
		return NULL;
	}

	const unsigned char *v = NULL;
	if (stmt && (SQLITE_ROW == sqlite3_step(stmt)))
		v = sqlite3_column_text(stmt, 0);
	gchar *ret = ( v ? g_strdup((char*) v) : NULL);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
		gel_warn("Cannot finalize query %s", q);
	sqlite3_free(q);
	
	return ret;
}

gboolean
adb_set_variable(Adb *self, gchar *variable, gchar *value)
{
	gchar *q = g_strdup_printf("UPDATE variables set value='%s' WHERE key='%s'", variable, value);

	char *error = NULL;
	gboolean ret = (sqlite3_exec(self->db, q, NULL, NULL, &error) == SQLITE_OK);
	if (!ret)
	{
		gel_error("Cannot update variable %s: %s. Query: %s", variable, error, q);
		sqlite3_free(error);
	}
	g_free(q);
	return ret;
}

gint adb_schema_get_version(Adb *self, gchar *schema)
{
	sqlite3_stmt *stmt = NULL;
	char *q = sqlite3_mprintf("SELECT version FROM schema_versions WHERE schema = '%q' LIMIT 1", schema);
	if (sqlite3_prepare_v2(self->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		sqlite3_free(q);
		return -2;
	}

	gint ret = -1;
	if (stmt && (SQLITE_ROW == sqlite3_step(stmt)))
		ret = sqlite3_column_int(stmt, 0);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
		gel_warn("Cannot finalize query %s", q);
	sqlite3_free(q);
	
	return ret;
}

void
adb_schema_set_version(Adb *self, gchar *schema, gint version)
{
	char *q = sqlite3_mprintf("INSERT OR REPLACE INTO schema_versions VALUES('%q',%d);", schema, version);
	char *error = NULL;
	if (sqlite3_exec(self->db, q, NULL, NULL, NULL) != SQLITE_OK)
	{
		gel_error("Cannot update schema version for %s: %s", schema, error);
		sqlite3_free(error);
	}
	sqlite3_free(q);
}

gboolean
adb_schema_upgrade(Adb *self, gchar *schema, gpointer *handlers, gpointer data, GError **error)
{
	gint current_version = adb_schema_get_version(self, schema);
	if (current_version == -2)
	{
		if (g_str_equal(schema, "core"))
		{
			gel_warn("First run, schema_versions table is not present, ignoring error");
			current_version++;
		}
		else
		{
			g_set_error(error, adb_quark(), ADB_ERROR_UPGRADING, N_("Cannot get schema version"));
			return FALSE;
		}
	}

	AdbUpgradeHandler handler;
	gint i = (current_version + 1);
	for (; handlers[i] != NULL; i++)
	{
		handler = handlers[i];
		if (!handler(self, data, error))
		{
			if (*error == NULL)
				g_set_error(error, adb_quark(), ADB_UNKNOW_ERROR, N_("Unknow error"));
			return FALSE;
		}

		adb_schema_set_version(self, schema, i);
	}

	return TRUE;
}

gboolean
adb_exec_queryes(Adb *self, gchar **queryes, gint *successes, GError **error)
{
	gint i;
	char *err = NULL;
	for (i = 0; queryes[i] != NULL; i++)
	{
		if (sqlite3_exec(self->db, queryes[i], NULL, NULL, &err) != SQLITE_OK)
		{
			g_set_error(error, adb_quark(), ADB_QUERY_ERROR,
				"%s: %s", queryes[i], err);
			sqlite3_free(err);
			break;
		}
	}
	if (successes != NULL)
		*successes = i;
	return (queryes[i] == NULL);
}


