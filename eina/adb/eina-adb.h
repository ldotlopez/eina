/*
 * eina/adb/eina-adb.h
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

#ifndef __EINA_ADB_H__
#define __EINA_ADB_H__

#include <glib-object.h>
#include <lomo/lomo-player.h>
#include <sqlite3.h>

G_BEGIN_DECLS

#define EINA_TYPE_ADB eina_adb_get_type()

#define EINA_ADB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ADB, EinaAdb))
#define EINA_ADB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_ADB, EinaAdbClass))
#define EINA_IS_ADB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ADB))
#define EINA_IS_ADB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_ADB))
#define EINA_ADB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_ADB, EinaAdbClass))

typedef struct {
	/* <private> */
	GObject parent;
} EinaAdb;

typedef struct {
	/* <private> */
	GObjectClass parent_class;
} EinaAdbClass;

typedef gboolean (*EinaAdbFunc)(EinaAdb *adb, GError **error);

enum {
	EINA_ADB_NO_ERROR = 0,
	EINA_ADB_ERROR_OBJECT_IS_NOT_ADB,
	EINA_ADB_ERROR_TRANSACTION,
	EINA_ADB_ERROR_QUERY_FAILED
};

#include "eina-adb-lomo.h"

GType eina_adb_get_type (void);

EinaAdb* eina_adb_new (void);

gchar    *eina_adb_get_db_file(EinaAdb *self);
gboolean  eina_adb_set_db_file(EinaAdb *self, const gchar *path);

// --
// Query queue
// --
void eina_adb_queue_query(EinaAdb *self, gchar *query, ...);
void eina_adb_flush(EinaAdb *self);

// --
// Easy API
// --

typedef struct _EinaAdbResult EinaAdbResult;

EinaAdbResult* eina_adb_query(EinaAdb *self, gchar *query, ...);
EinaAdbResult* eina_adb_query_raw(EinaAdb *self, gchar *query);
gboolean       eina_adb_query_exec(EinaAdb *self, const gchar *query, ...);
gboolean       eina_adb_query_exec_raw(EinaAdb *self, const gchar *query);
gboolean       eina_adb_query_block_exec(EinaAdb *self, gchar *queries[], GError **error);

gint eina_adb_changes(EinaAdb *self);

gint     eina_adb_result_column_count(EinaAdbResult *result);
gboolean eina_adb_result_step(EinaAdbResult *result);
gboolean eina_adb_result_get (EinaAdbResult *result, ...);
void     eina_adb_result_free(EinaAdbResult *result);

gchar    *eina_adb_get_variable(EinaAdb *self, gchar *variable);
gboolean  eina_adb_set_variable(EinaAdb *self, gchar *variable, gchar *value);

gint eina_adb_schema_get_version(EinaAdb *self, const gchar *schema);
void eina_adb_schema_set_version(EinaAdb *self, const gchar *schema, gint version);

gboolean eina_adb_upgrade_schema(EinaAdb *self, const gchar *schema, EinaAdbFunc callbacks[], GError **error);

sqlite3* eina_adb_get_handler(EinaAdb *self);

G_END_DECLS

#endif /* __EINA_ADB_H__ */
