/*
 * plugins/adb/upgrade.c
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
#include "upgrade.h"
#include <sqlite3.h>
#include <gel/gel.h>

static gboolean
adb_db_upgrade_0(Adb *self, GError **error)
{
	const gchar *querys[] = {
		"DROP TABLE IF EXISTS variables;",

		"CREATE TABLE variables ("
		"key VARCHAR(30) PRIMARY KEY,"
		"value VARCHAR(30)"
		");",

		"DROP TABLE IF EXISTS streams;",

		"CREATE TABLE streams ("
		"sid INTEGER PRIMARY KEY AUTOINCREMENT,"
		"uri VARCHAR(1024) UNIQUE,"
		"timestamp TIMESTAMP);",
		
		"INSERT INTO variables VALUES('schema-version', 0);",

		NULL };

	return adb_exec_querys(self, querys, NULL, error);
}

static gboolean
adb_db_upgrade_1(Adb *self, GError **error)
{
	const gchar *querys[] = {
		"DROP TABLE IF EXISTS tags;",

		"CREATE TABLE tags ("
		"tid INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name VARCHAR(63) UNIQUE"
		");",

		"CREATE TABLE metadata ("
		"tid INTEGER,"
		"sid INTEGER,"
		"value VARCHAR(63),"
		"PRIMARY KEY (tid,sid),"
		"FOREIGN KEY (tid) REFERENCES tags(tid),"
		"FOREIGN KEY (sid) REFERENCES streams(sid)"
		");",

		NULL
	};
	return adb_exec_querys(self, querys, NULL, error);
}

gboolean
adb_db_upgrade(Adb *self, gint from_version)
{
	gint i;
	gpointer upgrade_funcs[] = {
		adb_db_upgrade_0,
		adb_db_upgrade_1,
		NULL
	};

	GError *error = NULL;
	for (i = from_version + 1; upgrade_funcs[i]; i++)
	{
		gboolean (*func)(Adb *self, GError **error);
		func = upgrade_funcs[i];
		if (func(self, &error) == FALSE)
		{
			gel_error("Failed to upgrade ADB from version %d to %d: %s", i, i+1, error->message);
			g_error_free(error);
			return FALSE;
		}

		gchar *val = g_strdup_printf("%d", i);
		if (!adb_set_variable(self, "schema-version", val))
		{
			gel_error("Cannot upgrade to version %s", val);
			g_free(val);
			return FALSE;
		}
		g_free(val);

		gel_error("Upgraded to version %d", i);
	}
	return TRUE;
}


gboolean
adb_run_chained_functions(Adb *self, gpointer *functions, gint *successes, GError **error)
{
	
}
