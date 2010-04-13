/*
 * plugins/adb/eina-adb-lomo.c
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

gboolean
eina_adb_lomo_stream_set_sid(EinaAdb *adb, LomoStream *stream)
{
	g_return_val_if_fail(EINA_IS_ADB(adb) && LOMO_IS_STREAM(stream), FALSE);

	EinaAdbResult *res = eina_adb_query(adb, "SELECT sid FROM streams WHERE uri='%q'", lomo_stream_get_tag(stream, LOMO_TAG_URI));
	g_return_val_if_fail(res != NULL, FALSE);
	eina_adb_result_step(res);

	gint sid = -1;
	gboolean ret = eina_adb_result_get(res, 0, G_TYPE_INT, &sid, -1);
	eina_adb_result_free(res);

	if (ret)
	{
		GValue *v = g_new0(GValue, 1);
		g_value_init(v, G_TYPE_INT);
		g_value_set_int(v, sid);
		g_object_set_data_full((GObject *) stream, "x-adb-sid", v, g_free);
	}

	return ret;
}

gint
eina_adb_lomo_stream_get_sid(EinaAdb *adb, LomoStream *stream)
{
	GValue *v = (GValue *) g_object_get_data((GObject *) stream, "x-adb-sid");
	if (v)
		return g_value_get_int(v);
	
	// Run query on db
	gchar *uri = (gchar *) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	g_warning(N_("Stream for URI '%s' has no key 'x-adb-sid', quering to ADB"), uri);
	
	EinaAdbResult *res = eina_adb_query(adb, "SELECT sid FROM streams WHERE uri = '%q'");
	g_return_val_if_fail(res, -1);
	eina_adb_result_step(res);

	gint sid = -1;
	eina_adb_result_get(res, 0, G_TYPE_INT, &sid, -1);
	eina_adb_result_free(res);
	g_return_val_if_fail(sid != -1, -1);

	// Save value
	v = g_new0(GValue, 1);
	g_value_init(v, G_TYPE_INT);
	g_value_set_int(v, sid);
	g_object_set_data_full((GObject *) stream, "x-adb-sid", v, g_free);
	return sid;
}


