/*
 * plugins/adb/adb.c
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

static void
adb_register_connect_lomo(Adb *self, LomoPlayer *lomo);
static void
adb_register_disconnect_lomo(Adb *self, LomoPlayer *lomo);
static void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, Adb *self);
static void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, Adb *self);
static void
lomo_add_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data);

void
adb_register_enable(Adb *self)
{
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
	gel_warn("Got schema version: %d", schema_version);
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
	g_signal_connect(lomo,      "add",         (GCallback) lomo_add_cb,        self);
}

// --
// Disconnect 'add' from lomo
// Connect 'plugin-init' from app
// Disconnect 'plugin-fini' from app
static void
adb_register_disconnect_lomo(Adb *self, LomoPlayer *lomo)
{
	if (lomo == NULL) return;

	g_signal_handlers_disconnect_by_func(lomo, lomo_add_cb, self);
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
lomo_add_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data)
{
#if 0
	Adb *self = (Adb *) data;
	char *errmsg = NULL;

	gchar *uri = (gchar*) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	gchar *q = sqlite3_mprintf(
		"INSERT OR REPLACE INTO streams VALUES("
			"(SELECT sid FROM streams WHERE uri='%q'),"
			"'%q',"
			"DATETIME('NOW'));",
			uri,
			uri);

	if (sqlite3_exec(self->db, q, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		gel_warn("Cannot insert %s into database: %s", lomo_stream_get_tag(stream, LOMO_TAG_URI), errmsg);
		sqlite3_free(errmsg);
	}
	sqlite3_free(q);
#endif
}

