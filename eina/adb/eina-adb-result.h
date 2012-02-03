/*
 * eina/adb/eina-adb-result.h
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

#ifndef _EINA_ADB_RESULT
#define _EINA_ADB_RESULT

#include <glib-object.h>
#include <sqlite3.h>

G_BEGIN_DECLS

#define EINA_TYPE_ADB_RESULT eina_adb_result_get_type()

#define EINA_ADB_RESULT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ADB_RESULT, EinaAdbResult))
#define EINA_ADB_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ADB_RESULT, EinaAdbResultClass))
#define EINA_IS_ADB_RESULT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ADB_RESULT))
#define EINA_IS_ADB_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ADB_RESULT))
#define EINA_ADB_RESULT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ADB_RESULT, EinaAdbResultClass))

typedef struct _EinaAdbResultPrivate EinaAdbResultPrivate;
typedef struct {
	/*<private>*/
	GObject parent;
	EinaAdbResultPrivate *priv;
} EinaAdbResult;

typedef struct {
	/*<private>*/
	GObjectClass parent_class;
} EinaAdbResultClass;

GType eina_adb_result_get_type (void);

EinaAdbResult* eina_adb_result_new (sqlite3_stmt *stmt);

gint     eina_adb_result_column_count(EinaAdbResult *result);
gboolean eina_adb_result_step        (EinaAdbResult *result);
void eina_adb_result_get         (EinaAdbResult *result, ...);
void eina_adb_result_get_valist  (EinaAdbResult *result, va_list var_args);
GValue *eina_adb_result_get_value(EinaAdbResult *result, gint column, GType type);

G_END_DECLS

#endif /* _EINA_ADB_RESULT */

