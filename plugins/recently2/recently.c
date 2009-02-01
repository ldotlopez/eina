/*
 * plugins/recently2/iface.c
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
	GtkTreeView  *tv;
	GtkListStore *model;
	Adb *adb;
} Recently;

typedef struct {
	gchar *timestamp;
	gchar *uri;
} PlaylistEntry;
static PlaylistEntry*
playlist_entry_new(gchar *timestamp, gchar *uri);
static void
playlist_entry_free(PlaylistEntry *playlist_entry);
static GHashTable*
playlist_entries_group_by_timestamp(GList *entries);

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

GQuark recently_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("recently");
	return ret;
}

/*
static void pp
(gpointer key, gpointer val, gpointer data)
{
	gel_warn("Key %s: %d", (gchar *) key, g_list_length((GList *) val));
}
*/
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

static void
recently_dock_update(Recently *self)
{
	gtk_list_store_clear(GTK_LIST_STORE(self->model));

	// char *q = "select timestamp,count(*) from playlist_history group by timestamp;";
	char *q = "select timestamp,uri from playlist_history;";
	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(self->adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot fetch playlist_history data");
		return;
	}

	GList *entries = NULL;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		entries = g_list_prepend(entries, playlist_entry_new(
				(gchar *) sqlite3_column_text(stmt, 0),
				(gchar *) sqlite3_column_text(stmt, 1)));
	}
	sqlite3_finalize(stmt);

	GHashTable *grouped = playlist_entries_group_by_timestamp(entries);
	GList      *stamps  = g_list_sort(g_hash_table_get_keys(grouped), (GCompareFunc) strcmp);
	GList *iter = stamps;
	while (iter)
	{
		GtkTreeIter titer;
		gtk_list_store_append((GtkListStore *) self->model, &titer);

		GList *items = g_hash_table_lookup(grouped, iter->data);
		const gchar *stamp_human = stamp_to_human((gchar *) iter->data);
		gchar *title = g_strdup_printf("%s: %d streams",stamp_human, g_list_length(items));
		gtk_list_store_set((GtkListStore *) self->model, &titer,
			RECENTLY_COLUMN_TIMESTAMP, iter->data,
			RECENTLY_COLUMN_TITLE, title,
			-1);
		g_free(title);

		iter = iter->next;
	}

	g_list_free(stamps);
	g_hash_table_destroy(grouped);
	gel_list_deep_free(entries, playlist_entry_free);
}

static GtkWidget *
recently_dock_create(Recently *self)
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

	/* g_signal_connect(self->tv, "row-activated",
		G_CALLBACK(on_recently_dock_row_activated), plugin); */
	sw = (GtkScrolledWindow *) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(self->tv));

	recently_dock_update(self);

	return GTK_WIDGET(sw);

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

	gel_warn("ADB object retrieved: %p", adb);
	Recently *self = g_new0(Recently, 1);
	self->adb = adb;

	GtkWidget *dock = recently_dock_create(self);
	gtk_widget_show_all(dock);
	eina_plugin_add_dock_widget(plugin, "recently", gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU), dock);

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

static PlaylistEntry*
playlist_entry_new(gchar *timestamp, gchar *uri)
{
	PlaylistEntry *entry = g_new0(PlaylistEntry, 1);
	if (timestamp)
		entry->timestamp = g_strdup(timestamp);
	if (uri)
		entry->uri= g_strdup(uri);
	return entry;
}

static void
playlist_entry_free(PlaylistEntry *entry)
{
	gel_free_and_invalidate(entry->timestamp, NULL, g_free);
	gel_free_and_invalidate(entry->uri, NULL, g_free);
	gel_free_and_invalidate(entry, NULL, g_free);
}

static GHashTable*
playlist_entries_group_by_timestamp(GList *entries)
{
	GHashTable *ret = g_hash_table_new(g_str_hash, g_str_equal);
	GList *iter = entries;
	while (iter)
	{
		PlaylistEntry *e = (PlaylistEntry *) iter->data;
		g_hash_table_replace(ret, e->timestamp, 
			g_list_append(g_hash_table_lookup(ret, e->timestamp), e->uri));
		iter = iter->next;
	}
	return ret;
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
