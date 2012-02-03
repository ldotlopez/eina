/*
 * eina/adb/eina-adb-result.c
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

#include "eina-adb-result.h"
#include <gel/gel.h>

G_DEFINE_TYPE (EinaAdbResult, eina_adb_result, G_TYPE_OBJECT)

struct _EinaAdbResultPrivate {
	sqlite3_stmt *stmt;
};

static void
eina_adb_result_dispose (GObject *object)
{
	EinaAdbResult *self = EINA_ADB_RESULT(object);

	gel_free_and_invalidate(self->priv->stmt, NULL, sqlite3_finalize);

	G_OBJECT_CLASS (eina_adb_result_parent_class)->dispose (object);
}

static void
eina_adb_result_class_init (EinaAdbResultClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaAdbResultPrivate));

	object_class->dispose = eina_adb_result_dispose;
}

static void
eina_adb_result_init (EinaAdbResult *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_ADB_RESULT, EinaAdbResultPrivate));
}

/**
 * eina_adb_result_new: (skip):
 * @stmt: SQLite3 stament
 *
 * Don't use this function
 *
 * Returns: The new object
 */
EinaAdbResult*
eina_adb_result_new (sqlite3_stmt *stmt)
{
	EinaAdbResult *self = g_object_new (EINA_TYPE_ADB_RESULT, NULL);
	self->priv->stmt = stmt;
	return self;
}

/**
 * eina_adb_result_column_count:
 * @result: an #EinaAdbResult
 *
 * Checks how many columns has the result
 *
 * Returns: The column count
 */
gint
eina_adb_result_column_count(EinaAdbResult *result)
{
	g_return_val_if_fail(EINA_IS_ADB_RESULT(result), -1);
	return sqlite3_column_count(result->priv->stmt);
}

/**
 * eina_adb_result_step:
 * @result: an #EinaAdbResult
 *
 * Fetchs one row at time. see sqlite3_step() for details
 *
 * Returns: %TRUE if data was fetched, %FALSE otherwise
 */
gboolean
eina_adb_result_step(EinaAdbResult *result)
{
	g_return_val_if_fail(EINA_IS_ADB_RESULT(result), FALSE);

	int ret = sqlite3_step(result->priv->stmt);
	if (ret == SQLITE_DONE)
		return FALSE;

	g_return_val_if_fail(ret == SQLITE_ROW, FALSE);

	return TRUE;
}

/**
 * eina_adb_result_get_valist:
 * @result: an #EinaAdbResult
 * @var_args: <type>va_list</type> of column/return location pairs
 *
 * See eina_adb_result_get(), this version takes a <type>va_list</type>
 * for language bindings to use.
 *
 * aRename to: eina_adb_result_get
 */
void
eina_adb_result_get_valist(EinaAdbResult *result, va_list var_args)
{
	g_return_if_fail(EINA_IS_ADB_RESULT(result));

	sqlite3_stmt *stmt = result->priv->stmt;

	gint column = va_arg(var_args, gint);
	while (column >= 0)
	{
		GType type = va_arg(var_args, GType);
		gchar **str;
		gint   *i;
		guint  *u;

		switch (type)
		{
		case G_TYPE_STRING:
			str = va_arg(var_args, gchar**);
			*str = g_strdup((gchar *) sqlite3_column_text(stmt, column));
			break;
		case G_TYPE_INT:
			i = va_arg(var_args, gint*);
			*i = (gint) sqlite3_column_int(stmt, column);
			break;
		case G_TYPE_UINT:
			u = va_arg(var_args, guint*);
			*u = (guint) sqlite3_column_int(stmt, column);
			break;
		default:
			g_warning("Unhandled type '%s' in %s. Aborting", g_type_name(type), __FUNCTION__);
			return;
		}
		column = va_arg(var_args, gint);
	}
}

/**
 * eina_adb_result_get:
 * @result: an #EinaAdbResult
 * @...: pairs of column number and value return locations,
 *     terminated by -1
 *
 * Fetches the results
 */
void
eina_adb_result_get(EinaAdbResult *result, ...)
{
	g_return_if_fail(EINA_IS_ADB_RESULT(result));

	va_list var_args;

	va_start (var_args, result);
	eina_adb_result_get_valist(result, var_args);
	va_end (var_args);
}

/**
 * eina_adb_result_get_value:
 * @result: an #EinaAdbResult
 * @column: Result column to fetch
 * @type: Defines the type of the column.
 *
 * Fetches data from current row (see eina_adb_result_step()). @type is needed
 * since there is no reliable way to guess it.
 *
 * Returns: (transfer full): Data from row at @column
 */
GValue*
eina_adb_result_get_value(EinaAdbResult *result, gint column, GType type)
{
	g_return_val_if_fail(EINA_IS_ADB_RESULT(result), NULL);
	g_return_val_if_fail(column >= 0, NULL);

	sqlite3_stmt *stmt = result->priv->stmt;

	GValue *ret = g_new0(GValue, 1);

	switch (type)
	{
	case G_TYPE_STRING:
		g_value_init(ret, type);
		g_value_set_string(ret, (gchar *) sqlite3_column_text(stmt, column));
		break;
	case G_TYPE_INT:
		g_value_init(ret, type);
		g_value_set_int(ret, (gint) sqlite3_column_int(stmt, column));
		break;
	case G_TYPE_UINT:
		g_value_init(ret, type);
		g_value_set_uint(ret, (guint) sqlite3_column_int(stmt, column));
		break;
	default:
		g_warning("Unhandled type '%s' in %s. Aborting", g_type_name(type), __FUNCTION__);
		return NULL;
	}

	return ret;
}

