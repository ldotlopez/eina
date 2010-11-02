/*
 * eina/adb/eina-adb-lomo.c
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

#include "eina-adb-lomo.h"
#include <glib/gi18n.h>

gint
eina_adb_lomo_stream_attach_sid(EinaAdb *adb, LomoStream *stream)
{
	g_return_val_if_fail(EINA_IS_ADB(adb), -1);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), -1);

	// Check if already have a SID
	GValue *v = (GValue *) g_object_get_data((GObject *) stream, "x-adb-sid");
	if ((v != NULL) && (g_value_get_int(v) >= 0))
		return g_value_get_int(v);

	// Try INSERT or IGNORE INTO streams
	gchar *uri = (gchar *) lomo_stream_get_tag(stream, LOMO_TAG_URI);

	if (!eina_adb_query_exec(adb, "INSERT OR IGNORE INTO streams (uri,timestamp) VALUES('%q',STRFTIME('%%s',DATETIME('now')));", uri))
	{
		g_warning(N_("Cannot INSERT OR IGNORE"));
		return -1;
	}

	gchar *q = NULL;
	if (eina_adb_changes(adb) == 0)
		q = sqlite3_mprintf("SELECT sid FROM streams WHERE uri='%q'", uri);
	else
		q = sqlite3_mprintf("SELECT LAST_INSERT_ROWID();");

	EinaAdbResult *res = NULL;
	gint sid = -1;
	if (
		!(res = eina_adb_query_raw(adb, q)) ||
		!eina_adb_result_step(res) ||
		!eina_adb_result_get(res, 0, G_TYPE_INT, &sid, -1))
	{
		sqlite3_free(q);
		if (res)
			eina_adb_result_free(res);
		g_warning(N_("Unable to retrieve sid for stream"));
		return -1;
	}
	eina_adb_result_free(res);
	sqlite3_free(q);

	g_return_val_if_fail(sid >= 0, -1);

	// Attach result
	v = g_new0(GValue, 1);
	g_value_init(v, G_TYPE_INT);
	g_value_set_int(v, sid);
	g_object_set_data_full((GObject *) stream, "x-adb-sid", v, g_free);

	return sid;
}

gint
eina_adb_lomo_stream_get_sid(EinaAdb *adb, LomoStream *stream)
{
	GValue *v = (GValue *) g_object_get_data((GObject *) stream, "x-adb-sid");
	if (v)
		return g_value_get_int(v);

	return -1;
}

