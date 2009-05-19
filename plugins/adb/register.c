/*
 * plugins/adb/register.c
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

#define GEL_DOMAIN "Eina::Plugin::Adb"
#include <eina/eina-plugin.h>
#include <plugins/adb/adb.h>
#include "register.h"

static void
adb_register_connect_lomo(Adb *self, LomoPlayer *lomo);
static void
adb_register_disconnect_lomo(Adb *self, LomoPlayer *lomo);
static void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, Adb *self);
static void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, Adb *self);
static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data);
static void
lomo_remove_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data);
static void
lomo_clear_cb(LomoPlayer *lomo, gpointer data);
void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, gpointer data);

gboolean
adb_register_setup_0(Adb *self, gpointer data, GError **error)
{
	gchar *q[] = {
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
	return adb_exec_queryes(self, q, NULL, error);
}

void
adb_register_enable(Adb *self)
{
	gpointer callbacks[] = {
		adb_register_setup_0,
		NULL
		};

	GError *error = NULL;
	if (!adb_schema_upgrade(self, "register", callbacks, NULL, &error))
	{
		gel_error("Cannot enable register: %s", error->message);
		g_error_free(error);
		return;
	}

	LomoPlayer *lomo = GEL_APP_GET_LOMO(self->app);
	if (lomo == NULL)
		g_signal_connect(self->app, "plugin-init", (GCallback) app_plugin_init_cb, self);
	else
		adb_register_connect_lomo(self, lomo);

}

void
adb_register_disable(Adb *self)
{
	LomoPlayer *lomo = GEL_APP_GET_LOMO(self->app);
	if (lomo != NULL)
		adb_register_disconnect_lomo(self, lomo);
}

// --
// Create table for registers
// --
void
adb_register_setup_db(Adb *self)
{
	gint   schema_version;
	gchar *schema_version_str = adb_variable_get(self, "schema-version");
	if (schema_version_str == NULL)
		schema_version = -1;
	else
	{
		schema_version = atoi((gchar *) schema_version_str);
		g_free(schema_version_str);
	}
}

// --
// Connect 'add' from lomo
// Disconnect 'plugin-init' form app
// Connect 'plugin-fini' from app
// --
static void
adb_register_connect_lomo(Adb *self, LomoPlayer *lomo)
{
	if (lomo == NULL) return;

	g_signal_handlers_disconnect_by_func(self->app, app_plugin_init_cb, self);
	g_signal_connect(self->app, "plugin-fini", (GCallback) app_plugin_fini_cb, self);
	g_signal_connect(lomo,      "insert",      (GCallback) lomo_insert_cb,     self);
	g_signal_connect(lomo,      "remove",      (GCallback) lomo_remove_cb,     self);
	g_signal_connect(lomo,      "clear",       (GCallback) lomo_clear_cb,      self);
	g_signal_connect(lomo,      "all-tags",    (GCallback) lomo_all_tags_cb,   self);
}

// --
// Disconnect 'add' from lomo
// Connect 'plugin-init' from app
// Disconnect 'plugin-fini' from app
static void
adb_register_disconnect_lomo(Adb *self, LomoPlayer *lomo)
{
	if (lomo == NULL) return;

	g_signal_handlers_disconnect_by_func(lomo, lomo_insert_cb, self);
	g_signal_handlers_disconnect_by_func(lomo, lomo_remove_cb, self);
	g_signal_handlers_disconnect_by_func(lomo, lomo_clear_cb, self);
	g_signal_handlers_disconnect_by_func(lomo, lomo_all_tags_cb, self);
	g_signal_connect(self->app, "plugin-init", (GCallback)  app_plugin_init_cb, self);
	g_signal_handlers_disconnect_by_func(self->app, app_plugin_fini_cb, self);
}

// --
// Handle load/unload of lomo plugin
// --
static void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, Adb *self)
{
	if (!g_str_equal(plugin->name, "lomo"))
		return;
	adb_register_connect_lomo(self, GEL_APP_GET_LOMO(app));
}

static void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, Adb *self)
{
	if (!g_str_equal(plugin->name, "lomo"))
		return;
	adb_register_disconnect_lomo(self, gel_app_shared_get(app, "lomo"));
}

// --
// Register each stream added
// --
static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data)
{
	Adb *self = (Adb *) data;

	self->pl = g_list_prepend(self->pl, g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_URI)));

	gchar *uri = (gchar*) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	gchar *q[2];
	q[0] = sqlite3_mprintf(
		"INSERT OR IGNORE INTO streams (uri,timestamp) VALUES("
			"'%q',"
			"DATETIME('NOW', 'UTC'));",
			uri);
	q[1] = NULL;

	GError *error = NULL;
	if (!adb_exec_queryes((Adb*) data, q, NULL, &error))
	{
		gel_error("%s", error->message);
		g_error_free(error);
	}
	sqlite3_free(q[0]);
}

static void
lomo_remove_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data)
{
	Adb *self = (Adb *) data;
	GList *p;
	if ((p = g_list_find_custom(self->pl, lomo_stream_get_tag(stream, LOMO_TAG_URI), (GCompareFunc) strcmp)) == NULL)
	{
		gel_warn("Deleted stream not found");
		return;
	}
	self->pl = g_list_remove_link(self->pl, p);
	g_free(p->data);
	g_list_free(p);
}

static void
lomo_clear_cb(LomoPlayer *lomo, gpointer data)
{

	Adb *self = (Adb *) data;
	char *err = NULL;
	GTimeVal now; g_get_current_time(&now);
	gchar *now_str = g_time_val_to_iso8601(&now);

	GDate dt;
	g_date_clear(&dt, 1);
	g_date_set_time_val(&dt, &now);

	if (sqlite3_exec(self->db, "BEGIN TRANSACTION;", NULL, NULL, &err) != SQLITE_OK)
	{
		gel_warn("Cannot begin transaction: %s", err);
		sqlite3_free(err);
		return;
	}

	self->pl = g_list_reverse(self->pl);
	GList *iter = self->pl;
	while (iter)
	{
		char *q = sqlite3_mprintf("INSERT INTO playlist_history VALUES('%s',(SELECT sid FROM streams WHERE uri='%q'));", now_str, iter->data);
		if (sqlite3_exec(self->db, q, NULL, NULL, &err) != SQLITE_OK)
		{
			gel_warn("Cannot update playlist_history: %s", err);
			sqlite3_exec(self->db, "ROLLBACK;", NULL, NULL, NULL);
			sqlite3_free(q);
			goto lomo_clear_cb_return;
		}
		sqlite3_free(q);
		iter = iter->next;
	}

	if (sqlite3_exec(self->db, "END TRANSACTION;", NULL, NULL, &err) != SQLITE_OK)
	{
		gel_warn("Cannot end transaction: %s", err);
		goto lomo_clear_cb_return;
	}
lomo_clear_cb_return:
	gel_free_and_invalidate(err, NULL, sqlite3_free);
	gel_free_and_invalidate(now_str, NULL, g_free);
	if (self->pl != NULL)
	{
		gel_list_deep_free(self->pl, g_free);
		self->pl = NULL;
	}
}


void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, gpointer data)
{

	Adb *self = (Adb *) data;
	char *err = NULL;

	if (sqlite3_exec(self->db, "BEGIN TRANSACTION;", NULL, NULL, &err) != SQLITE_OK)
	{
		gel_warn("Cannot begin transaction: %s", err);
		sqlite3_free(err);
		return;
	}

	gchar *uri = (gchar *) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	GList *tags = lomo_stream_get_tags(stream);
	GList *iter = tags;
	while (iter)
	{
		gchar *tag = iter->data;
		if (lomo_tag_get_type(tag) != G_TYPE_STRING)
		{
			iter = iter->next;
			continue;
		}

		gchar *value = (gchar *) lomo_stream_get_tag(stream, iter->data);
		char *q = sqlite3_mprintf("INSERT OR IGNORE INTO metadata "
			"VALUES((SELECT sid FROM streams WHERE uri='%q'), '%q', '%q');", uri, tag, value);
		if (sqlite3_exec(self->db, q, NULL, NULL, &err) != SQLITE_OK)
		{
			gel_warn("Cannot store metadata %s for %s: %s", tag, uri, err);
			sqlite3_free(err);
			err = NULL;
		}

		iter = iter->next;
	}
	g_list_free(tags);

	if (sqlite3_exec(self->db, "END TRANSACTION;", NULL, NULL, &err) != SQLITE_OK)
		gel_warn("Cannot end transaction: %s", err);

}

