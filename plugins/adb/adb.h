/*
 * plugins/adb/adb.h
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

#ifndef __ADB_H
#define __ADB_H

#include <sqlite3.h>
#include <gel/gel.h>

G_BEGIN_DECLS

#define GEL_APP_GET_ADB(app) ((Adb*) gel_app_shared_get(app, "adb"))

#define EINA_ADB(p) ((Adb *) p)
#define eina_obj_get_adb(obj) gel_app_get_adb(eina_obj_get_app(obj))
#define gel_app_get_adb(app) ((Adb*) gel_app_shared_get(app, "adb"))

typedef struct Adb {
	sqlite3 *db;
	GelApp  *app;
	GList   *pl;
} Adb;

typedef gboolean (*AdbUpgradeHandler)(Adb *self, gpointer data, GError **error);

enum {
	ADB_NO_ERROR = 0,
	EINA_ADB_ERROR_VERSION_MISMATCH,
	ADB_CANNOT_GET_LOMO,
	ADB_CANNOT_REGISTER_OBJECT,
	ADB_QUERY_ERROR,
	ADB_ERROR_UPGRADING,
	ADB_UNKNOW_ERROR
};

GQuark adb_quark(void);

// --
// Create / Destroy adb object
// --
Adb*
adb_new(GelApp *app, GError **error);
void
adb_free(Adb *self);

// --
// Variables
// --
gchar *
adb_variable_get(Adb *self, gchar *variable);
gboolean
adb_set_variable(Adb *self, gchar *variable, gchar *value);

// --
// Schemas
// --
gint
adb_schema_get_version(Adb *self, gchar *schema);
void
adb_schema_set_version(Adb *self, gchar *schema, gint version);
gboolean
adb_schema_upgrade(Adb *self, gchar *schema, gpointer *handlers, gpointer data, GError **error);

// --
// Querying
// --
gboolean
adb_exec_queryes(Adb *self, gchar **queryes, gint *successes, GError **error);

G_END_DECLS

#endif // _ADB_COMMON_H
