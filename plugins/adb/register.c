/*
 * plugins/adb/register.c
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

#include "eina-adb.h"
#include <string.h>
#include <lomo/lomo-player.h>
#include <gel/gel.h>

GEL_DEFINE_WEAK_REF_CALLBACK(adb_register)

static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self);
static void
lomo_remove_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self);
static void
lomo_clear_cb(LomoPlayer *lomo, EinaAdb *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaAdb *self);

static gchar *schema_1[] = {
		"DROP TABLE IF EXISTS streams;",
		"CREATE TABLE streams ("
		"	sid INTEGER PRIMARY KEY AUTOINCREMENT,"
		"	uri VARCHAR(1024) UNIQUE NOT NULL,"
		"	timestamp TIMESTAMP NOT NULL,"
		"	played TIMESTAMP DEFAULT 0,"
		"	count INTEGER DEFAULT 0"
		");",
		"CREATE INDEX streams_sid_idx ON streams(sid);",

		"DROP TABLE IF EXISTS metadata;",
		"CREATE TABLE metadata ("
		"	sid INTEGER NOT NULL,"
		"	key VARCHAR(128),"
		"	value VARCHAR(128),"
		"	CONSTRAINT metadata_pk PRIMARY KEY(sid,key),"
		"	CONSTRAINT metadata_sid_fk FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE"
		");",
		"CREATE INDEX metadata_key_idx ON metadata(key);",

		"DROP TABLE IF EXISTS playlist_history;",
		"CREATE TABLE playlist_history("
		"	timestamp TIMESTAMP NOT NULL,"
		"	sid INTEGER,"
		"	CONSTRAINT playlist_history_pk PRIMARY KEY(timestamp,sid),"
		"	CONSTRAINT playlist_history_sid_fk FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE"
		");",

		NULL
};

static gchar *schema_2[] = {
		"DROP VIEW IF EXISTS fast_meta;",
		"CREATE VIEW fast_meta AS"
		"  SELECT t.sid AS sid, t.value AS title, a.value AS artist, b.value AS album FROM"
		"    (SELECT sid,value FROM metadata WHERE key='artist') AS a JOIN"
		"    (SELECT sid,value FROM metadata WHERE key='album')  AS b USING(sid) JOIN"
		"    (SELECT sid,value FROM metadata WHERE key='title')  AS t USING(sid);",

		NULL
};

static gchar **schema_queries[] = { schema_1, schema_2, NULL };

static GList *__playlist = NULL;

struct {
	gchar *signal;
	gpointer handler;
} signal_table[] = {
	{ "insert",   lomo_insert_cb   },
	{ "remove",   lomo_remove_cb   },
	{ "clear",    lomo_clear_cb    },
	{ "all-tags", lomo_all_tags_cb },
	{ NULL, NULL }
};

void
adb_register_start(EinaAdb *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_ADB(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	g_return_if_fail(eina_adb_upgrade_schema(self, "register", schema_queries, NULL));

	g_object_ref(lomo);
	g_object_weak_ref((GObject *) lomo, adb_register_weak_ref_cb, NULL);

	gint i;
	for (i = 0; signal_table[i].signal != NULL; i++)
		g_signal_connect(lomo, signal_table[i].signal, (GCallback) signal_table[i].handler, self);
}

void
adb_register_stop(EinaAdb *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_ADB(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	gint i;
	for (i = 0; signal_table[i].signal != NULL; i++)
		g_signal_handlers_disconnect_by_func(lomo, signal_table[i].handler, self);

	g_object_weak_unref((GObject *) lomo, adb_register_weak_ref_cb, NULL);
	g_object_unref(lomo);
}

static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));

	gchar *uri = (gchar*) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	eina_adb_queue_query(self, sqlite3_mprintf(
		"INSERT OR IGNORE INTO streams (uri,timestamp) VALUES("
			"'%q',"
			"DATETIME('NOW', 'UTC'));",
			uri));
	__playlist = g_list_prepend(__playlist, g_strdup(uri));
}

static void
lomo_remove_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));

	GList *p;
	if ((p = g_list_find_custom(__playlist, lomo_stream_get_tag(stream, LOMO_TAG_URI), (GCompareFunc) strcmp)) == NULL)
	{
		g_warning("Deleted stream not found in internal playlist");
		return;
	}
	__playlist = g_list_remove_link(__playlist, p);
	g_free(p->data);
	g_list_free(p);
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));

	if (!__playlist) return;

	GTimeVal now; g_get_current_time(&now);
	gchar *now_str = g_time_val_to_iso8601(&now);

	GDate dt;
	g_date_clear(&dt, 1);
	g_date_set_time_val(&dt, &now);

	GList *iter = __playlist = g_list_reverse(__playlist);
	while (iter)
	{
		eina_adb_queue_query(self, sqlite3_mprintf(
			"INSERT INTO playlist_history VALUES('%s',(SELECT sid FROM streams WHERE uri='%q'));", now_str, iter->data));
		g_free(iter->data);
		iter = iter->next;
	}
	g_list_free(__playlist);
	__playlist = NULL;

	g_free(now_str);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));

	gchar *uri = (gchar *) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	GList *tags = lomo_stream_get_tags(stream);
	GList *iter = tags;
	while (iter)
	{
		gchar *tag = iter->data;
		if (lomo_tag_get_g_type(tag) != G_TYPE_STRING)
		{
			iter = iter->next;
			continue;
		}

		gchar *value = (gchar *) lomo_stream_get_tag(stream, iter->data);
		eina_adb_queue_query(self, sqlite3_mprintf("INSERT OR IGNORE INTO metadata "
			"VALUES((SELECT sid FROM streams WHERE uri='%q'), '%q', '%q');", uri, tag, value));
		iter = iter->next;
	}
	gel_list_deep_free(tags, (GFunc) g_free);
}


