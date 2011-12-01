/*
 * eina/adb/eina-adb-upgrade.c
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
#include <glib/gi18n.h>
#include <gel/gel.h>

/**
 * eina_adb_upgrade_schema:
 * @self: An #EinaAdb
 * @schema: Schema name
 * @callbacks: (array zero-terminated=1) : set of callbacks
 * @error: Location for errors
 *
 * Does magic
 *
 * Returns: %TRUE on successful, %FALSE otherwise
 */
gboolean
eina_adb_upgrade_schema(EinaAdb *self, const gchar *schema, EinaAdbFunc callbacks[], GError **error)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);

	gint schema_version = eina_adb_schema_get_version(self, schema);

	// g_warning("Current '%s' schema: %d", schema, schema_version);
	gint i;
	for (i = schema_version + 1; callbacks[i] != NULL; i++)
	{
		g_warning("Upgrade ADB from DB schema #%d to #%d. Using callbacks from callbacks[%d]", i, i+1, i);
		if (!callbacks[i](self, error))
			break;
		eina_adb_schema_set_version(self, schema, i);
	}

	return (callbacks[i] == NULL);
}

