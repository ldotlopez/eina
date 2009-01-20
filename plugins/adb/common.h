/*
 * plugins/adb/common.h
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

#ifndef _ADB_COMMON_H
#define _ADB_COMMON_H
#include <glib.h>
#include "adb.h"

G_BEGIN_DECLS

gboolean
adb_exec_querys(Adb *self, const gchar **querys, gint *success, GError **error);

gboolean
adb_set_variable(Adb *self, gchar *variable, gchar *value);

gint
adb_table_get_schema_version(Adb *self, gchar *table);

G_END_DECLS

#endif // _ADB_COMMON_H
