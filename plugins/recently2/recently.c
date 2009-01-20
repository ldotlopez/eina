/*
 * plugins/recently2/recently.c
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

#define GEL_DOMAIN "Eina::Plugins::Recently"
#include <plugins/adb/adb.h>
#include "recently.h"

struct _Recently {
	Adb *adb;
};

GQuark recently_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("recently");
	return ret;
}

Recently*
recently_new(GelApp *app, GError **error)
{
	Adb *adb = GEL_APP_GET_ADB(app);
	if (adb == NULL)
	{
		g_set_error(error, recently_quark(), RECENTLY_NO_ADB_OBJECT,
			N_("Cannot get adb object"));
		return NULL;
	}

	// Update recently table
	gint schema_version = adb_table_get_schema_version(adb, "recently");
	gel_warn("Current version for recently table: %d", schema_version);


	Recently *self = g_new(Recently, 1);
	self->adb = adb;

	gel_warn("ok, recently schema version: %d", adb_table_get_schema_version(adb, "recently"));
	return self;
}
