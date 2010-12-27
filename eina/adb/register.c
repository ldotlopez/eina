/*
 * eina/adb/register.c
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

#include "register.h"
#include "eina-adb.h"
#include <string.h>
#include <sys/time.h>
#include <lomo/lomo-player.h>
#include <lomo/lomo-util.h>
#include <gel/gel.h>

GEL_DEFINE_WEAK_REF_CALLBACK(adb_register)

#define debug(...) do ; while(0)
// #define debug(...) g_warning(__VA_ARGS__)

static inline void
reset_counters(void);
static inline void
set_checkpoint(gint64 check_point, gboolean add);

static gboolean
lomo_insert_hook(LomoPlayer *lomo, LomoPlayerHookEvent event, gpointer ret, EinaAdb *adb);

static void
lomo_state_change_cb(LomoPlayer *lomo);
static void
lomo_eos_cb(LomoPlayer *lomo, EinaAdb *adb);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to);
static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to);
static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self);
static void
lomo_remove_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self);
static void
lomo_clear_cb(LomoPlayer *lomo, EinaAdb *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaAdb *self);

static gboolean
upgrade_1(EinaAdb *self, GError **error)
{
	gchar *qs[] = {
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
	return eina_adb_query_block_exec(self, qs, error);
}

static gboolean
upgrade_2(EinaAdb *self, GError **error)
{
	gchar *qs[] = {
		"DROP VIEW IF EXISTS fast_meta;",
		"CREATE VIEW fast_meta AS"
		"  SELECT t.sid AS sid, t.value AS title, a.value AS artist, b.value AS album FROM"
		"    (SELECT sid,value FROM metadata WHERE key='artist') AS a JOIN"
		"    (SELECT sid,value FROM metadata WHERE key='album')  AS b USING(sid) JOIN"
		"    (SELECT sid,value FROM metadata WHERE key='title')  AS t USING(sid);",

		NULL
	};
	return eina_adb_query_block_exec(self, qs, error);
};

// Added for Eina 0.9.4
static gboolean
upgrade_3(EinaAdb *self, GError **error)
{
	gchar *qs[] = {
		"DROP TABLE IF EXISTS recent_plays;",
		"CREATE TABLE recent_plays ("
		"	sid INTEGER NOT NULL,"
		"	timestamp TIMESTAMP NOT NULL,"
		"	CONSTRAINT recent_plays_pk PRIMARY KEY(sid,timestamp),"
		"	CONSTRAINT recent_plays_fk FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE"
		");",

		"DROP TABLE IF EXISTS historic_plays;",
		"CREATE TABLE historic_plays ("
		"	sid INTEGER NOT NULL,"
		"	timestamp TIMESTAMP NOT NULL,"
		"	aggregation_timestamp TIMESTAMP NOT NULL,"
		"	count INTEGER NOT NULL DEFAULT 0,"
		"	CONSTRAINT historic_plays_pk PRIMARY KEY(sid),"
		"	CONSTRAINT historic_plays_fk FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE"
		");",

		"DROP TABLE IF EXISTS stream_relationship;",
		"CREATE TABLE stream_relationship ("
		"	sid  INTEGER NOT NULL,"
		"	sid2 INTEGER NOT NULL,"
		"	CONSTRAINT stream_relationship_sid_fk  FOREIGN KEY(sid)  REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE,"
		"	CONSTRAINT stream_relationship_sid2_fk FOREIGN KEY(sid2) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE"
		");",
		"DROP INDEX IF EXISTS stream_relationship_sid_idx;"
		"DROP INDEX IF EXISTS stream_relationship_sid2_idx;"
		"CREATE INDEX stream_relationship_sid_idx  ON stream_relationship(sid);"
		"CREATE INDEX stream_relationship_sid2_idx ON stream_relationship(sid2);"
		
		"UPDATE streams SET timestamp = STRFTIME('%s', timestamp);",
		"UPDATE streams SET played    = STRFTIME('%s', played) where played > 0;",
		"UPDATE playlist_history SET timestamp = STRFTIME('%s', timestamp);",
		"INSERT OR REPLACE INTO variables VALUES('timestamp-format', 'unixepoch');",

		NULL
	};
	return eina_adb_query_block_exec(self, qs, error);
};

static EinaAdbFunc upgrade_funcs[] = { upgrade_1, upgrade_2, upgrade_3, NULL };


// Our data
static GList *__playlist = NULL;
static struct {
	gboolean    submited;
	gint64      played;
	gint64      check_point;
	gboolean    submit;
} __markers;

struct {
	gchar *signal;
	gpointer handler;
} __signal_table[] = {
	{ "play",       lomo_state_change_cb },
	{ "pause",      lomo_state_change_cb },
	{ "stop",       lomo_state_change_cb },
	{ "pre-change", lomo_eos_cb      },
	{ "eos",        lomo_eos_cb      },
	{ "change",     lomo_change_cb   },
	{ "seek",       lomo_seek_cb     },
	{ "insert",     lomo_insert_cb   },
	{ "remove",     lomo_remove_cb   },
	{ "clear",      lomo_clear_cb    },
	{ "all-tags",   lomo_all_tags_cb },
	{ NULL, NULL }
};

void
adb_register_start(EinaAdb *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_ADB(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	g_return_if_fail(eina_adb_upgrade_schema(self, "register", upgrade_funcs, NULL));

	g_object_ref(lomo);
	g_object_weak_ref((GObject *) lomo, adb_register_weak_ref_cb, NULL);

	GList *iter = (GList *) lomo_player_get_playlist(lomo);
	while (iter)
	{
		eina_adb_lomo_stream_attach_sid(self, LOMO_STREAM(iter->data));
		iter = iter->next;
	}

	lomo_player_hook_add(lomo, (LomoPlayerHook) lomo_insert_hook, self);

	gint i;
	for (i = 0; __signal_table[i].signal != NULL; i++)
		g_signal_connect(lomo, __signal_table[i].signal, (GCallback) __signal_table[i].handler, self);
}

void
adb_register_stop(EinaAdb *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_ADB(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	gint i;
	for (i = 0; __signal_table[i].signal != NULL; i++)
		g_signal_handlers_disconnect_by_func(lomo, __signal_table[i].handler, self);
	lomo_player_hook_remove(lomo, (LomoPlayerHook) lomo_insert_hook);

	g_object_weak_unref((GObject *) lomo, adb_register_weak_ref_cb, NULL);
	g_object_unref(lomo);
}

static inline void
reset_counters(void)
{
    debug("Reseting counters");
	__markers.submited = FALSE;
	__markers.played = 0;
	__markers.check_point = 0;
}

static inline void
set_checkpoint(gint64 check_point, gboolean add)
{
	debug("Set checkpoint: %"G_GINT64_FORMAT" secs (%d)", lomo_nanosecs_to_secs(check_point), add);
	if (add)
		__markers.played += (check_point - __markers.check_point);
	__markers.check_point = check_point;
	debug("  Currently %"G_GINT64_FORMAT" secs played", lomo_nanosecs_to_secs(__markers.played));
}

static gboolean
lomo_insert_hook(LomoPlayer *lomo, LomoPlayerHookEvent event, gpointer ret, EinaAdb *adb)
{
	if (event.type != LOMO_PLAYER_HOOK_INSERT)
		return FALSE;
	g_return_val_if_fail(EINA_IS_ADB(adb), FALSE);
	g_return_val_if_fail(LOMO_IS_STREAM(event.stream), FALSE);

	eina_adb_lomo_stream_attach_sid(adb, event.stream);
	return FALSE;
}

static void
lomo_state_change_cb(LomoPlayer *lomo)
{
	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		// Set checkpoint without acumulate
		set_checkpoint(lomo_player_tell_time(lomo), FALSE);
		break;

	case LOMO_STATE_STOP:
		debug("stop signal, position may be 0");
	
	case LOMO_STATE_PAUSE:
		// Add to counter secs from the last checkpoint
		set_checkpoint(lomo_player_tell_time(lomo), TRUE);
		break;

	default:
		// Do nothing?
		debug("Unknow state. Be careful");
		break;
	}
}

static void
lomo_eos_cb(LomoPlayer *lomo, EinaAdb *adb)
{
	debug("Got EOS/PRE_CHANGE");
	set_checkpoint(lomo_player_tell_time(lomo), TRUE);

	if ((__markers.played >= 30) && (__markers.played >= (lomo_player_length_time(lomo) / 2)) && !__markers.submited)
	{
		debug("Submit to lastfm");
		gint sid = eina_adb_lomo_stream_get_sid(adb, lomo_player_get_current_stream(lomo));
		g_return_if_fail(sid >= 0);
		eina_adb_query_exec(adb, "UPDATE streams SET played = STRFTIME('%%s', 'NOW'), count = (count + 1) WHERE sid=%d;", sid);
		eina_adb_query_exec(adb, "INSERT INTO recent_plays (sid,timestamp) VALUES(%d,STRFTIME('%%s', 'NOW'))", sid);
		__markers.submited = TRUE;
	}
	else
		debug("Not enought to submit to lastfm");
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to)
{
	reset_counters();
}

static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to)
{
	// Count from checkpoint to 'from'
	// Move checkpoint to 'to' without adding
	set_checkpoint(from, TRUE);
	set_checkpoint(to,   FALSE);
}

static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self)
{
	g_return_if_fail(LOMO_IS_STREAM(stream));
	gint sid = eina_adb_lomo_stream_get_sid(self, stream);
	g_return_if_fail(sid >= 0);

	GList *iter = __playlist;

	eina_adb_queue_query(self, "BEGIN TRANSACTION;");
	while (iter)
	{
		eina_adb_queue_query(self, "INSERT INTO stream_relationship VALUES (%d,%d);", sid, GPOINTER_TO_INT(iter->data));
		eina_adb_queue_query(self, "INSERT INTO stream_relationship VALUES (%d,%d);", GPOINTER_TO_INT(iter->data), sid);
		iter = iter->next;
	}
	eina_adb_queue_query(self, "COMMIT TRANSACTION;");

	__playlist = g_list_prepend(__playlist, GINT_TO_POINTER(sid));
}

static void
lomo_remove_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));

	GList *p;
	if ((p = g_list_find(__playlist, GINT_TO_POINTER(eina_adb_lomo_stream_get_sid(self, stream)))) == NULL)
	{
		g_warning("Deleted stream not found in internal playlist");
		return;
	}
	__playlist = g_list_remove_link(__playlist, p);
	g_list_free(p);
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaAdb *self)
{
	g_return_if_fail(EINA_IS_ADB(self));

	if (!__playlist) return;

	time_t now;
	struct tm *ts;
	char buffer[80];

	now = time(NULL);
	ts = localtime(&now);
	strftime(buffer, sizeof(buffer), "%s", ts);

	GList *iter = __playlist = g_list_reverse(__playlist);
	while (iter)
	{
		eina_adb_queue_query(self,
			"INSERT INTO playlist_history VALUES('%s',%d);", buffer, GPOINTER_TO_INT(iter->data));
		iter = iter->next;
	}

	g_list_free(__playlist);
	__playlist = NULL;
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
		if (lomo_tag_get_gtype(tag) != G_TYPE_STRING)
		{
			iter = iter->next;
			continue;
		}

		gchar *value = (gchar *) lomo_stream_get_tag(stream, iter->data);
		eina_adb_queue_query(self, "INSERT OR IGNORE INTO metadata "
			"VALUES((SELECT sid FROM streams WHERE uri='%q'), '%q', '%q');", uri, tag, value);
		iter = iter->next;
	}
	gel_list_deep_free(tags, (GFunc) g_free);
}


