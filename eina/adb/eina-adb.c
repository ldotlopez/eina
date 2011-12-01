/*
 * eina/adb/eina-adb.c
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

#include "eina-adb.h"
#include <string.h>
#include <sqlite3.h>
#include <gel/gel.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (EinaAdb, eina_adb, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ADB, EinaAdbPrivate))

GEL_DEFINE_QUARK_FUNC(eina_adb);

#define SQLITE3_STMT(x) ((sqlite3_stmt *) x)
typedef sqlite3_stmt _EinaAdbResult;

typedef struct _EinaAdbPrivate EinaAdbPrivate;
struct _EinaAdbPrivate {
	gchar      *db_file;
	sqlite3    *db;
	LomoPlayer *lomo;
	GQueue     *queue;
	GList      *playlist;
	guint       flush_id;
};

enum {
	PROPERTY_DB_FILE = 1,
};

static gboolean
upgrade_1(EinaAdb *self, GError **error)
{
	gchar *queries[] =
	{
	// Control schemas versions
	"DROP TABLE IF EXISTS schema_versions;"
	"CREATE TABLE schema_versions ("
	"   schema VARCHAR(32) PRIMARY KEY,"
	"   version INTERGER"
	");",

	// Variables table
	"DROP TABLE IF EXISTS variables;"
	"CREATE TABLE variables ("
	"   key VARCHAR(256) PRIMARY KEY,"
	"   value VARCHAR(1024)"
	");",

	NULL
	};

	return eina_adb_query_block_exec(self, queries, error);
};

static EinaAdbFunc upgrade_funcs[] = { &upgrade_1, NULL };

static gboolean
adb_flush(EinaAdb *self);
static void
adb_schedule_flush(EinaAdb *self);

static void
eina_adb_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	EinaAdb *self = EINA_ADB(object);

	switch (property_id) {
	case PROPERTY_DB_FILE:
		g_value_set_string(value, eina_adb_get_db_file(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_adb_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaAdb *self = EINA_ADB(object);  

	switch (property_id) {
	case PROPERTY_DB_FILE:
		eina_adb_set_db_file(self, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_adb_dispose (GObject *object)
{
	EinaAdb *self = EINA_ADB(object);
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	if (priv->queue)
	{
		adb_flush(self);
		g_queue_free(priv->queue);
		priv->queue = NULL;
	}
	G_OBJECT_CLASS (eina_adb_parent_class)->dispose (object);
}

static void
eina_adb_class_init (EinaAdbClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaAdbPrivate));

	object_class->get_property = eina_adb_get_property;
	object_class->set_property = eina_adb_set_property;
	object_class->dispose = eina_adb_dispose;

	g_object_class_install_property(object_class, PROPERTY_DB_FILE,
		g_param_spec_string("db-filename", "db-filename",  "db-filename",
		NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
eina_adb_init (EinaAdb *self)
{
	EinaAdbPrivate *priv = GET_PRIVATE(self);
	priv->queue = g_queue_new();
}

EinaAdb*
eina_adb_new (void)
{
	return g_object_new (EINA_TYPE_ADB, NULL);
}

gchar *
eina_adb_get_db_file(EinaAdb *self)
{
	g_return_val_if_fail(EINA_IS_ADB(self), NULL);
	return g_strdup(GET_PRIVATE(self)->db_file);
}

gboolean
eina_adb_set_db_file(EinaAdb *self, const gchar *path)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->db_file, NULL, g_free);
	gel_free_and_invalidate(priv->db, NULL, sqlite3_close);

	if (!path)
		return TRUE;

	priv->db_file = g_strdup(path);
	int code = sqlite3_open(priv->db_file, &priv->db);
	if (code != SQLITE_OK)
	{
		g_warning("Unable to open sqlite3 database '%s': %d", path, code);
		eina_adb_set_db_file(self, NULL);
		return FALSE;
	}

	GError *error = NULL;
	if (!eina_adb_upgrade_schema(self, "core", upgrade_funcs, &error))
	{
		g_warning("Unable to upgrade database '%s': %s", priv->db_file, error ? error->message : N_("No error message"));
		if (error)
			g_error_free(error);
		eina_adb_set_db_file(self, NULL);
		return FALSE;
	}

	return TRUE;
}

//
// Easy query
//
/**
 * eina_adb_query: (skip):
 * @self: An #EinaAdb
 * @query: Query to execute in sqlite format, see %sqlite3_vmprintf
 *
 * Builds and executes query
 *
 * Returns: (transfer none): An #EinaAdbResult
 */
EinaAdbResult*
eina_adb_query(EinaAdb *self, gchar *query, ...)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);
	g_return_val_if_fail(query != NULL, NULL);

	va_list args;
	va_start(args, query);
	gchar *q = sqlite3_vmprintf(query, args);
	va_end(args);

	g_return_val_if_fail(q != NULL, NULL);

	EinaAdbResult *res = NULL;
	if (!(res = eina_adb_query_raw(self, q)))
	{
		sqlite3_free(q);
		g_return_val_if_fail(res != NULL, NULL);
	}

	sqlite3_free(q);
	return res;
}

/**
 * eina_adb_query_raw:
 * @self: An #EinaAdb
 * @query: Query to execute
 *
 * Executes query
 *
 * Returns: (transfer none): An #EinaAdbResult
 */
EinaAdbResult*
eina_adb_query_raw(EinaAdb *self, gchar *query)
{
	g_return_val_if_fail(EINA_IS_ADB(self), NULL);
	g_return_val_if_fail(query != NULL, NULL);

	EinaAdbPrivate *priv = GET_PRIVATE(self);

	int code;
	EinaAdbResult *res = NULL;
	if ((code = sqlite3_prepare_v2(priv->db, query, -1, (sqlite3_stmt **) &res, NULL)) != SQLITE_OK)
		g_warning("Query failed with code %d, query was: '%s'", code, query);
	return res;
}

/**
 * eina_adb_query_exec_raw:
 * @self: A #EinaAdb
 * @query: A query
 *
 * Executes @query
 *
 * Returns: %TRUE on successful, %FALSE othewise
 */
gboolean
eina_adb_query_exec_raw(EinaAdb *self, const gchar *query)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);
	g_return_val_if_fail((query != NULL), FALSE);
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	char *msg = NULL;
	int ret = sqlite3_exec(priv->db, query, NULL, NULL, &msg);

	if (ret == 0)
		return TRUE;
	else
	{
		g_warning(N_("Error %d in query '%s'"), ret, msg);
		sqlite3_free(msg);
		return FALSE;
	}
}

gboolean
eina_adb_query_exec(EinaAdb *self, const gchar *query, ...)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);

	va_list args;
	va_start(args, query);
	gchar *_query = sqlite3_vmprintf(query, args);
	va_end(args);
	g_return_val_if_fail((query != NULL), FALSE);

	gboolean ret = eina_adb_query_exec_raw(self, _query);
	sqlite3_free(_query);

	return ret;
}

/**
 * eina_adb_query_block_exec:
 * @self: An #EinaAdb
 * @queries: (element-type utf8) (array zero-terminated=1): Array of queries
 * @error: Location for errors
 *
 * Executes safelly a block of queries. A ROLLBACK is executed if any of them fails
 *
 * Returns: %TRUE on successful, %FALSE othewise
 */
gboolean
eina_adb_query_block_exec(EinaAdb *self, gchar *queries[], GError **error)
{
	if (!EINA_IS_ADB(self))
	{
		g_set_error(error, eina_adb_quark(), EINA_ADB_ERROR_OBJECT_IS_NOT_ADB,
			"EINA_IS_ADB(self) failed");
		return FALSE;
	}

	if (!eina_adb_query_exec(self, "BEGIN TRANSACTION;"))
	{
		g_set_error(error, eina_adb_quark(), EINA_ADB_ERROR_TRANSACTION,
			N_("Cannot begin transaction"));
		return FALSE;
	}

	gint i;
	for (i = 0; queries[i] != NULL; i++)
	{
		if (!eina_adb_query_exec(self, queries[i]))
		{
			g_set_error(error, eina_adb_quark(), EINA_ADB_ERROR_QUERY_FAILED,
				N_("Query %d failed. Query was: %s"), i, queries[i]);
			eina_adb_query_exec(self, "ROLLBACK;");
			return FALSE;
		}
	}

	if (!eina_adb_query_exec(self, "END TRANSACTION;"))
	{
		g_set_error(error, eina_adb_quark(), EINA_ADB_ERROR_TRANSACTION,
			N_("Cannot end transaction"));
		return FALSE;
	}

	return TRUE;
}

gint
eina_adb_changes(EinaAdb *self)
{
	g_return_val_if_fail(EINA_IS_ADB(self), -1);
	return (gint) sqlite3_changes(GET_PRIVATE(self)->db);
}

gint
eina_adb_result_column_count(EinaAdbResult *result)
{
	g_return_val_if_fail(result != NULL, -1);
	return sqlite3_column_count(SQLITE3_STMT(result));
}

gboolean
eina_adb_result_step(EinaAdbResult *result)
{
	g_return_val_if_fail(result != NULL, FALSE);

	int ret = sqlite3_step(SQLITE3_STMT(result));
	if (ret == SQLITE_DONE)
		return FALSE;

	g_return_val_if_fail(ret == SQLITE_ROW, FALSE);

	return TRUE;
}

gboolean
eina_adb_result_get(EinaAdbResult *result, ...)
{
	va_list args;
	va_start(args, result);
	gint column = va_arg(args, gint);
	while (column >= 0)
	{
		GType type = va_arg(args, GType);
		gchar **str;
		gint   *i;
		guint  *u;

		switch (type)
		{
		case G_TYPE_STRING:
			str = va_arg(args, gchar**);
			*str = g_strdup((gchar *) sqlite3_column_text(SQLITE3_STMT(result), column));
			break;
		case G_TYPE_INT:
			i = va_arg(args, gint*);
			*i = (gint) sqlite3_column_int(SQLITE3_STMT(result), column);
			break;
		case G_TYPE_UINT:
			u = va_arg(args, guint*);
			*u = (guint) sqlite3_column_int(SQLITE3_STMT(result), column);
			break;
		default:
			g_warning(N_("Unhandled type '%s' in %s. Aborting"), g_type_name(type), __FUNCTION__);
			return FALSE;
		}
		column = va_arg(args, guint); 
	}
	return TRUE;
}

void
eina_adb_result_free(EinaAdbResult *result)
{
	g_return_if_fail(result != NULL);
	sqlite3_finalize(SQLITE3_STMT(result));
}

// --
// Queue querys
// --
void
eina_adb_queue_query(EinaAdb *self, gchar *query, ...)
{
	g_return_if_fail(EINA_IS_ADB(self));
	g_return_if_fail(query != NULL);
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	va_list args;
	va_start(args, query);
	gchar *q = sqlite3_vmprintf(query, args);
	va_end(args);

	g_queue_push_tail(priv->queue, q);
	adb_schedule_flush(self);
}

static gboolean
adb_flush(EinaAdb *self)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	if (!priv->db) return FALSE;

	char *q = NULL;
	while ((q = g_queue_pop_head(priv->queue)) != NULL)
	{
		char *err = NULL;
		int code = sqlite3_exec(priv->db, q, NULL, NULL, &err);
		if (code != SQLITE_OK)
		{
			g_warning("Error while executing query '%s': %s", q, err);
			sqlite3_free(err);
		}

		sqlite3_free(q);
	}

	priv->flush_id = 0;
	return FALSE;
}

static void
adb_schedule_flush(EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	if (priv->flush_id)
		g_source_remove(priv->flush_id);
	priv->flush_id = g_timeout_add_seconds(5, (GSourceFunc) adb_flush, self);
}

// --
// Variable mini-API
// --
gchar *
eina_adb_get_variable(EinaAdb *self, gchar *variable)
{
	g_return_val_if_fail(EINA_IS_ADB(self), NULL);
	g_return_val_if_fail(variable != NULL,  NULL);

	EinaAdbResult *res = eina_adb_query(self, "SELECT value FROM variables WHERE key = '%q' LIMIT 1", variable);
	if (!res)
	{
		g_warning(N_("Error in query for get variable '%s'"), variable);
		return NULL;
	}

	gchar *ret = NULL;
	if (!eina_adb_result_get(res, 0, G_TYPE_STRING, &ret, -1))
	{
		g_warning(N_("Error getting variable '%s'"), variable);
		eina_adb_result_free(res);
		return NULL;
	}

	eina_adb_result_free(res);
	return ret;
}

gboolean
eina_adb_set_variable(EinaAdb *self, gchar *variable, gchar *value)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);
	g_return_val_if_fail(variable != NULL,  FALSE);
	g_return_val_if_fail(value    != NULL,  FALSE);

	gboolean ret = eina_adb_query_exec(self, "UPDATE variables set value='%s' WHERE key='%s'", variable, value);
	if (!ret)
	{
		g_warning(N_("Error setting variable '%s'='%s'"), variable, value);
		return FALSE;
	}
	return TRUE;
}

gint
eina_adb_schema_get_version(EinaAdb *self, const gchar *schema)
{
	g_return_val_if_fail(EINA_IS_ADB(self), -1);
	g_return_val_if_fail(schema != NULL, -1);
	EinaAdbPrivate *priv = GET_PRIVATE(self);

	// Dont use easy-api, we want low level access
    sqlite3_stmt *stmt = NULL;
	char *q = sqlite3_mprintf("SELECT version FROM schema_versions WHERE schema = '%q' LIMIT 1;", schema);

	int code = sqlite3_prepare_v2(priv->db, q, -1, &stmt, NULL);
	if (code != SQLITE_OK)
	{
		sqlite3_free(q);
		return -1; // No schema, not installed
	}

	gint ret = -1;
	if (stmt && (SQLITE_ROW == sqlite3_step(stmt)))
		ret = sqlite3_column_int(stmt, 0);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
		g_warning(N_("Cannot finalize query"));
    sqlite3_free(q);

	return ret;
}

void
eina_adb_schema_set_version(EinaAdb *self, const gchar *schema, gint version)
{
	g_return_if_fail(EINA_IS_ADB(self));
	g_return_if_fail(schema != NULL);

	if (!eina_adb_query_exec(self, "INSERT OR REPLACE INTO schema_versions VALUES('%q',%d);", schema, version))
	{
		g_warning(N_("Error setting schema version for '%s'=%d"), schema, version);
		return;
	}
}

/**
 * eina_adb_get_handler: (skip):
 * @self: An #EinaAdb
 *
 * Returns the low level sqlite3 handler for the bbdd
 *
 * Returns: (transfer none): Then sqlite3 handler
 */
sqlite3*
eina_adb_get_handler(EinaAdb *self)
{
	return GET_PRIVATE(self)->db;
}

