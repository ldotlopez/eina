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

#include <config.h>

#define GEL_DOMAIN "Eina::Plugin::Recently"
#define EINA_PLUGIN_DATA_TYPE "Recently"
#define EINA_PLUGIN_NAME "Recently"

#include <string.h>
#include <eina/eina-plugin.h>
#include "plugins/adb/adb.h"

typedef struct {
	GelApp       *app;
	GelPlugin    *plugin;
	GtkWidget    *dock;
	GtkTreeView  *tv;
	GtkListStore *model;
} Recently;

enum {
	RECENTLY_COLUMN_TIMESTAMP,
	RECENTLY_COLUMN_TITLE,
	RECENTLY_COLUMNS
};

enum {
	RECENTLY_NO_ERROR,
	RECENTLY_ERROR_CANNOT_LOAD_ADB,
	RECENTLY_ERROR_CANNOT_UNLOAD_ADB
};

// --
// Utils
// --
static gchar*
stamp_to_human(gchar *stamp);
static gchar *
summary_playlist(Adb *adb, gchar *timestamp, guint how_many);

// --
// plugin init/fini connect/disconnect lomo
// --
void
connect_lomo(Recently *self);
void
disconnect_lomo(Recently *self);
void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, Recently *self);
void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, Recently *self);

void
lomo_clear_cb(LomoPlayer *lomo, Recently *self);

// --
// Dock related
// --
static GtkWidget *
dock_create(Recently *self);
static void
dock_update(Recently *self);
void
dock_row_activated_cb(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	Recently *self);

GQuark recently_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("recently");
	return ret;
}

// Convert a string in iso8601 format to a more human form
static gchar*
stamp_to_human(gchar *stamp)
{
	GTimeVal now, other;
	g_get_current_time(&now);
	if (!g_time_val_from_iso8601(stamp, &other))
	{
		gel_warn("Invalid input");
		return NULL;
	}
	gchar *weekdays[] = {
		NULL,
		N_("Monday"),
		N_("Tuesday"),
		N_("Wednesday"),
		N_("Thursday"),
		N_("Saturday"),
		N_("Sunday")
	};
	gint diff = ((now.tv_sec - other.tv_sec) / 60 / 60 / 24);

	if (diff == 0)
		return N_("Today");
	else if (diff == 1)
		return N_("Yesterday");
	else if ((diff >= 2) && (diff <= 6))
	{
		GDate *d = g_date_new();
		g_date_set_time_val(d, &other);
		gchar *ret = weekdays[g_date_get_weekday(d)];
		g_date_free(d);
		return ret;
	}
	else if ((diff >= 7) && (diff <= 30))
		return N_("More than 7 days ago");
	else if ((diff >= 31) && (diff <= 365))
		return N_("More than a month ago");
	else
		return N_("More than a year ago");
}

static gchar *
summary_playlist(Adb *adb, gchar *timestamp, guint how_many)
{
	sqlite3_stmt *stmt = NULL;
	char *q = sqlite3_mprintf(
	"SELECT COUNT(*) AS count,value FROM ("
	"	SELECT sid,uri FROM"
	"		playlist_history AS pl JOIN streams USING(sid) WHERE pl.timestamp='%q') JOIN"
	"		metadata USING(sid) WHERE key='artist' GROUP BY value ORDER BY count desc LIMIT %d", timestamp, how_many);
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_warn("Cannot summaryze %s: %s", timestamp, sqlite3_errmsg(adb->db));
		return NULL;
	}

	GList *items = NULL;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		items = g_list_prepend(items, g_strdup((gchar *) sqlite3_column_text(stmt, 1)));
	}
	items = g_list_reverse(items);
	
	GString *out = g_string_new(NULL);
	GList *iter = items;
	while (iter)
	{
		out = g_string_append(out, iter->data);
		g_free(iter->data);

		if (iter->next)
		{
			if (iter->next->next)
				out = g_string_append(out, N_(", "));
			else
				out = g_string_append(out, N_(" and "));
		}
		iter = iter->next;
	}
	g_list_free(items);

	gchar *ret = out->str;
	g_string_free(out, FALSE);
	return ret;
}

void
connect_lomo(Recently *self)
{
	LomoPlayer *lomo = LOMO_PLAYER(gel_app_shared_get(self->app, "lomo"));
	if ((lomo == NULL) || !LOMO_IS_PLAYER(lomo))
		return;
	g_signal_connect(lomo, "clear", (GCallback) lomo_clear_cb, self);
}

void
disconnect_lomo(Recently *self)
{
	LomoPlayer *lomo = LOMO_PLAYER(gel_app_shared_get(self->app, "lomo"));
	if ((lomo == NULL) || !LOMO_IS_PLAYER(lomo))
		return;
	g_signal_handlers_disconnect_by_func(lomo, lomo_clear_cb, self);
}
void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, Recently *self)
{
	if (g_str_equal(plugin->name, "lomo"))
		connect_lomo(self);
}

void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, Recently *self)
{
	if (g_str_equal(plugin->name, "lomo"))
		disconnect_lomo(self);
}

void
lomo_clear_cb(LomoPlayer *lomo, Recently *self)
{
	dock_update(self);
}

// --
// Dock related
// --
static GtkWidget *
dock_create(Recently *self)
{
	GtkScrolledWindow *sw;
	GtkTreeViewColumn *col, *col2;
	GtkCellRenderer   *render;

	self->tv = GTK_TREE_VIEW(gtk_tree_view_new());
	render = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Timestamp",
		render, "text", RECENTLY_COLUMN_TIMESTAMP, NULL);
	col2 = gtk_tree_view_column_new_with_attributes("Description",
		render, "markup", RECENTLY_COLUMN_TITLE, NULL);

	gtk_tree_view_append_column(self->tv, col);
	gtk_tree_view_append_column(self->tv, col2);

	self->model = gtk_list_store_new(RECENTLY_COLUMNS,
		G_TYPE_STRING,
		G_TYPE_STRING);

	g_object_set(G_OBJECT(render),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_END,
		NULL);
	g_object_set(G_OBJECT(col),
		"visible",   TRUE,
		"resizable", FALSE,
		NULL);
	g_object_set(G_OBJECT(col2),
		"visible",   TRUE,
		"resizable", FALSE,
		NULL);
    g_object_set(G_OBJECT(self->tv),
		"search-column", -1,
		"headers-clickable", FALSE,
		"headers-visible", TRUE,
		NULL);

    gtk_tree_view_set_model(self->tv, GTK_TREE_MODEL(self->model));

	g_signal_connect(self->tv, "row-activated",
		G_CALLBACK(dock_row_activated_cb), self);
	sw = (GtkScrolledWindow *) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(self->tv));

	dock_update(self);

	return GTK_WIDGET(sw);
}

static void
dock_update(Recently *self)
{
	gtk_list_store_clear(GTK_LIST_STORE(self->model));

	Adb *adb = (Adb*) gel_app_shared_get(self->app, "adb");
	if (adb == NULL)
		return;

	char *q = "SELECT DISTINCT(timestamp) FROM playlist_history WHERE timestamp > 0 ORDER BY timestamp DESC;";
	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot fetch playlist_history data");
		return;
	}
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		gchar *ts = (gchar *) sqlite3_column_text(stmt, 0);
		gchar *summary = summary_playlist(adb, ts, 3);
		gchar *escaped = g_markup_escape_text(summary, -1);
		g_free(summary);

		GtkTreeIter iter;
		gtk_list_store_append((GtkListStore *) self->model, &iter);

		gchar *title = g_strdup_printf("<b>%s:</b>\n\t%s ", stamp_to_human(ts), escaped);
		g_free(escaped);

		gtk_list_store_set((GtkListStore *) self->model, &iter,
			RECENTLY_COLUMN_TIMESTAMP, ts,
			RECENTLY_COLUMN_TITLE, title,
			-1);
		g_free(title);
	}
	sqlite3_finalize(stmt);
}

void
dock_row_activated_cb(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	Recently *self)
{
	GtkTreeIter iter;
	gchar *ts;

	Adb *adb = (Adb *) gel_app_shared_get(self->app, "adb");
	if (adb == NULL)
		return;
	LomoPlayer *lomo = (LomoPlayer *) gel_app_shared_get(self->app, "lomo");
	if (lomo == NULL)
		return;
	
	gtk_tree_model_get_iter(GTK_TREE_MODEL(self->model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(self->model), &iter,
		RECENTLY_COLUMN_TIMESTAMP, &ts,
		-1);

	char *q = sqlite3_mprintf(
		"SELECT uri FROM streams WHERE sid IN ("
			"SELECT sid FROM playlist_history WHERE timestamp='%q'"
		");", ts);
	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_warn("Cannot get URIs for %s: %s", ts, sqlite3_errmsg(adb->db));
		sqlite3_free(q);
		return;
	}

	GList *pl = NULL;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		pl = g_list_prepend(pl, g_strdup((gchar*) sqlite3_column_text(stmt, 0)));
	}
	sqlite3_finalize(stmt);
	pl = g_list_reverse(pl);

	q = sqlite3_mprintf("DELETE FROM playlist_history WHERE timestamp='%q'", ts);
	char *err = NULL;
	if (sqlite3_exec(adb->db, q, NULL, NULL, &err) != SQLITE_OK)
	{
		gel_error("Cannot delete playlist %s form history: %s", ts, err);
		sqlite3_free(err);
	}
	g_free(ts);

	lomo_player_clear(lomo);
	lomo_player_add_uri_multi(lomo, pl);
	gel_list_deep_free(pl, g_free);

	eina_plugin_switch_dock_widget(self->plugin, "playlist");
	dock_update(self);

	lomo_player_play(lomo, NULL);
}

gboolean
recently_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	// Load adb
	GError *err = NULL;
	if (!gel_app_load_plugin_by_name(app, "adb", &err))
	{
		g_set_error(error, recently_quark(), RECENTLY_ERROR_CANNOT_LOAD_ADB, 
			N_("Cannot load adb plugin: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}

	Adb *adb;
	if ((adb = gel_app_shared_get(app, "adb")) == NULL)
		return FALSE;

	Recently *self = g_new0(Recently, 1);
	self->app = app;
	self->plugin = plugin;

	self->dock = dock_create(self);
	gtk_widget_show(self->dock);
	eina_plugin_add_dock_widget(plugin, "recently", gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU), self->dock);

	plugin->data = self;

	return TRUE;
}

gboolean
recently_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	GError *err = NULL;
	if (!gel_app_unload_plugin_by_name(app, "adb", &err))
	{
		g_set_error(error, recently_quark(), RECENTLY_ERROR_CANNOT_UNLOAD_ADB, 
			N_("Cannot unload adb plugin: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}
	return TRUE;
}

EinaPlugin recently2_plugin = {
	EINA_PLUGIN_SERIAL,
	"recently", PACKAGE_VERSION,
	N_("Stores your recent playlists"),
	N_("Recently plugin 2. ADB based version"), // long desc
	NULL, // icon
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	recently_plugin_init, recently_plugin_fini,
	NULL, NULL
};
