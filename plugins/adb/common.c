/*
 * plugins/adb/common.c
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

#define GEL_DOMAIN "Eina::ADB:Common"

#include "common.h"
#include <sqlite3.h>
#include <gel/gel.h>

GQuark
adb_gquark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-adb-error-quark");
	return ret;
}

gboolean
adb_exec_querys(Adb *self, const gchar **querys, gint *success, GError **error)
{
	gint i;
	for (i = 0; querys[i] != NULL; i++)
	{
		char *errmsg = NULL;
		gint ret = sqlite3_exec(self->db, querys[i], NULL, NULL, &errmsg);
		if (ret != SQLITE_OK)
		{
			gel_error("ADB got error %d: %s. Query: %s", ret, errmsg, querys[i]);
			g_set_error_literal(error, adb_gquark(), ret, errmsg);
			sqlite3_free(errmsg);
			break;
		}
	}
	if (success)
		*success = i;
	return (querys[i] == NULL);
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

gint
adb_table_get_schema_version(Adb *self, gchar *table)
{
	// Check for table
	char *q = sqlite3_mprintf("SELECT * FROM %s LIMIT 0", table);
	if (sqlite3_exec(self->db, q, NULL, NULL, NULL) != SQLITE_OK)
	{
		sqlite3_free(q);
		return -1;
	}
	sqlite3_free(q);

	// Table exits, query for version

	sqlite3_stmt *stmt = NULL;
	q = sqlite3_mprintf("SELECT value FROM variables WHERE key='%q-schema-version' LIMIT 1", table);
	gel_warn("Q: %s", q);
	if (sqlite3_prepare_v2(self->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		sqlite3_free(q);
		return -2;
	}
	sqlite3_free(q);

	if (stmt && (SQLITE_ROW == sqlite3_step(stmt)))
	{	
		gint version = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
		return version;
	}
	return -3;
}

