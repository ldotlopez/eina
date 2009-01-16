/*
 * plugins/recently/recently.c
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

#define GEL_DOMAIN "Eina::Plugin::Recently"
#define PLUGIN_NAME "recently"
#define EINA_PLUGIN_DATA_TYPE RecentlyData

#include <string.h>
#include <glib/gstdio.h>
#include "recently.h"
#include "scores.h"

typedef struct RecentlyData {
	GtkTreeView *tv;
	GtkListStore *model;
	GtkWidget *dock_label, *dock_widget;
} RecentlyData;

enum {
	RECENTLY_COLUMN_TITLE,
	RECENTLY_COLUMN_PLAYLIST_FILE,
	RECENTLY_COLUMNS
};

/*
 * Widget definitions 
 */
GtkWidget*
recently_dock_new(EinaPlugin *plugin);
void
recently_dock_update(EinaPlugin *plugin);
void
on_recently_dock_row_activated(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	EinaPlugin *plugin);

// Lomo Callbacks
void
on_recently_lomo_clear(LomoPlayer *lomo, EinaPlugin *self);

// Agrupates playlist by tag, returns a hash with results
GHashTable *
recently_agrupate_playlist_by_tag(GList *pl, LomoTag tag)
{
	GList *iter = pl;
	GHashTable *ret;
	gpointer *key, *val;
	gint i;

	ret = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	eina_iface_info("Agrupating playlist %p by tag %s", pl, tag);
	while (iter)
	{
		key = (gpointer) lomo_stream_get_tag(LOMO_STREAM(iter->data), tag);
		if (key != NULL)
		{
			if ((val = g_hash_table_lookup(ret, key)) == NULL)
				i = 1;
			else
				i = GPOINTER_TO_INT(val) + 1;
			g_hash_table_insert(ret, g_strdup((gchar *) key), GINT_TO_POINTER(i));
		}
		iter = iter->next;
	}

	iter = g_hash_table_get_keys(ret);
	eina_iface_info("Agrupation done. %p has %d groups", ret, g_list_length(iter));
	g_list_free(iter);

	return ret;
}

// Creates a new list using only the tag 'tag' of each LomoStream from playlist
// 'pl', dont free data from playlist
GList *
recently_playlist_extract_tag(GList *pl, LomoTag tag)
{
	GList *ret = NULL, *iter = pl;
	gint nulls = 0;
	const gchar *data;

	eina_iface_info("Extracting tag '%s' from %p", tag, pl);
	while (iter) {
		data = lomo_stream_get_tag(LOMO_STREAM(iter->data), tag);
		if (data == NULL)
			nulls++;
		ret = g_list_prepend(ret, (gpointer) data);

		iter = iter->next;
	}

	eina_iface_info("Extraction done. Parsed %d streams, nulls: %d", g_list_length(pl), nulls);
	return g_list_reverse(ret);
}

// Dumps current playlist to our store. This function also summarizes playlist
// and saves meta-file with the statistics of the playlist.
gboolean
currently_playlist_store(EinaPlugin *plugin)
{
	LomoPlayer *lomo = EINA_PLUGIN_LOMO(plugin);
	GList  *lomo_pl, *list;
	gchar  *buffer;
	GTimeVal  now;
	gchar    *now_str;
	gchar    *pl_file, *meta_file;

	GHashTable *agrupated_pl;
	GList *score_records;

	eina_iface_debug("LomoPlayer at: %p", lomo);
	lomo_pl = (GList *) lomo_player_get_playlist(lomo);

	// Build buffer to dump into a playlist file
	list = recently_playlist_extract_tag(lomo_pl, LOMO_TAG_URI);
	if (list == NULL)
		return FALSE;

	buffer = gel_list_join("\n", list);
	if (buffer == NULL)
	{
		g_list_free(list);
		return FALSE;
	}
	g_list_free(list);

	// Now dump to file name like 'now' (2008-08-22T00:21:44Z)
	g_get_current_time(&now);
	now_str = g_time_val_to_iso8601(&now);

	// pl_file = eina_iface_build_plugin_filename(PLUGIN_NAME, now_str);
	pl_file = eina_plugin_build_userdir_path(plugin, now_str);
	gel_warn("Userdir path: '%s'", pl_file);

	meta_file = g_strconcat(pl_file, ".meta", NULL);
	g_free(now_str);

	g_file_set_contents(pl_file, buffer, -1, NULL);
	eina_iface_info("Playlist saved to '%s'", pl_file);
	g_free(pl_file);
	g_free(buffer);

	// Dump stats
	agrupated_pl = recently_agrupate_playlist_by_tag(lomo_pl, LOMO_TAG_ARTIST);
	score_records = recently_score_record_list_from_hash(agrupated_pl);
	recently_score_record_file_write(score_records, meta_file);
	eina_iface_info(" metadata wrote to '%s'", meta_file);
	gel_list_deep_free(score_records, (GFunc) recently_score_record_free, NULL);

	return TRUE;
}

gboolean
recently_playlist_unlink(EinaPlugin *plugin, gchar *id)
{
	gboolean ret = TRUE;

	// gchar *pl_path   = eina_iface_build_plugin_filename(PLUGIN_NAME, id);
	gchar *pl_path   = eina_plugin_build_userdir_path(plugin, id);
	gchar *meta_path = g_strconcat(pl_path, ".meta", NULL);

	if (g_unlink(pl_path) == -1)
		ret = FALSE;
	if (g_unlink(meta_path) == -1)
		ret = FALSE;
	eina_iface_info("Deleted playlist '%s' and metadata '%s'", pl_path, meta_path);
	g_free(pl_path);
	g_free(meta_path);
	return ret;
}

// Compares two playlist filenames if they are iso8601 based
gint recently_compare_playlists(gchar *a, gchar *b)
{
	gchar *a2, *b2;
	GTimeVal a_tv, b_tv;
	gint ret = 0;

	a2 = g_strdup(a);
	b2 = g_strdup(b);

	if (g_str_has_suffix(a2, ".meta"))
	{
		a2[strlen(a2) - 5] = '\0';
		b2[strlen(b2) - 5] = '\0';
	}

	if (!g_time_val_from_iso8601((const gchar *) a2, &a_tv))
	{
		ret = -1;
		goto recently_compare_playlists_exit;
	}

	if (!g_time_val_from_iso8601((const gchar *) b2, &b_tv))
	{
		ret = 1;
		goto recently_compare_playlists_exit;
	}

	if (a_tv.tv_sec < b_tv.tv_sec)
	{
		ret = -1;
		goto recently_compare_playlists_exit;
	}
	if (a_tv.tv_sec > b_tv.tv_sec)
	{
		ret = 1;
		goto recently_compare_playlists_exit;
	}

recently_compare_playlists_exit:
	g_free(a2);
	g_free(b2);
	return ret;
}

GList *recently_get_playlists(EinaPlugin *plugin)
{
	GList *entries = NULL;
	GDir *d;
	const gchar *entry;

	// gchar *plugin_data_dir = eina_iface_get_plugin_dir(PLUGIN_NAME);
	gchar *plugin_data_dir = eina_plugin_build_userdir_path(plugin, NULL);

	d = g_dir_open((const gchar *) plugin_data_dir, 0, NULL);
	if (d == NULL)
		return NULL;

	while ((entry = g_dir_read_name(d)) != NULL) {
		if (!g_str_has_suffix(entry, ".meta"))
			continue;
		entries = g_list_prepend(entries, g_strdup(entry));
	}
	g_dir_close(d);
	return g_list_reverse(g_list_sort(entries, (GCompareFunc) recently_compare_playlists));
}

gchar *
recently_dock_format_playlist_data(GList *playlist_data)
{
	GString *str = g_string_new(NULL);
	gchar *ret;

	GList *iter = playlist_data;
	while (iter)
	{
		RecentlyScoreRecord *record = (RecentlyScoreRecord *) iter->data; 
		str = g_string_append(str, record->id);
		if (iter->next)
		{
			if (iter->next->next)
				str = g_string_append(str, ", ");
			else
				str = g_string_append(str, " and ");
		}

		iter = iter->next;
	}

	ret = g_markup_escape_text(str->str, -1);
	g_string_free(str, TRUE);
	return ret;
}

gchar *
recently_calculate_time_diff(GTimeVal now, GTimeVal prev)
{
	gchar *weekday_table[] = {
		"Groundhog Day", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"
	};
	glong diff = now.tv_sec - prev.tv_sec;
	gchar *human_time_diff = NULL;

	if (diff < 0)
	{
		human_time_diff = g_strdup("Back from the future");
	}
	else if (diff <= (24*60*60))
	{
		human_time_diff = g_strdup("Today");
	}
	else if (diff <= (48*60*60))
	{
		human_time_diff = g_strdup("Yesterday");
	}
	else if (diff <= (7 * 24*60*60))
	{
		GDate *date = g_date_new();
		g_date_set_time_val(date, &prev);
		
		human_time_diff = g_strdup_printf("On %s", weekday_table[g_date_get_weekday(date)]);
		g_date_free(date);
	}
	else if (diff <= (15 * 24*60*60))
	{
		human_time_diff = g_strdup("In the last 7 days");
	}
	else 
	{
		human_time_diff = g_strdup("Previously since last week");	
	}
	return human_time_diff;
}

GtkWidget *
recently_dock_new(EinaPlugin *plugin)
{
	RecentlyData *self = EINA_PLUGIN_DATA(plugin);
	GtkScrolledWindow *sw;
	GtkTreeViewColumn *col, *col2;
	GtkCellRenderer   *render;

	self->tv = GTK_TREE_VIEW(gtk_tree_view_new());
	render = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Recent playlists",
	        render, "markup", RECENTLY_COLUMN_TITLE, NULL);
	col2 = gtk_tree_view_column_new_with_attributes(NULL,
	        render, "text", RECENTLY_COLUMN_PLAYLIST_FILE, NULL);

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
		"visible",   FALSE,
		NULL);
    g_object_set(G_OBJECT(self->tv),
		"search-column", -1,
		"headers-clickable", FALSE,
		"headers-visible", TRUE,
		NULL);

    gtk_tree_view_set_model(self->tv, GTK_TREE_MODEL(self->model));
	recently_dock_update(plugin);

	g_signal_connect(self->tv, "row-activated",
		G_CALLBACK(on_recently_dock_row_activated), plugin);
	sw = (GtkScrolledWindow *) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(self->tv));

	return GTK_WIDGET(sw);
}

void
recently_dock_update(EinaPlugin *plugin)
{
	RecentlyData *self = EINA_PLUGIN_DATA(plugin);
	GList *recent_playlists = NULL, *l;
	GtkTreeIter iter;
	GList *playlist_data;
	GTimeVal now;

	l = recent_playlists = recently_get_playlists(plugin);

	g_get_current_time(&now);
	gtk_list_store_clear(self->model);
	while (l)
	{
		gchar *tmp, *tmp2, *diff_str;
		gchar *pl_file_basename;
		GTimeVal when;
		gchar *when_str;

		// gchar *filename = eina_iface_build_plugin_filename(PLUGIN_NAME, (gchar *) l->data);
		gchar *filename = eina_plugin_build_userdir_path(plugin, (gchar *) l->data);

		playlist_data = recently_score_record_file_read(filename);
		when_str = g_path_get_basename(filename);
		pl_file_basename = g_strdup(when_str);
		pl_file_basename[strlen(pl_file_basename) - 5] = '\0';
		g_free(filename);

		g_time_val_from_iso8601((const gchar *) when_str, &when);
		g_free(when_str);

		tmp = recently_dock_format_playlist_data(playlist_data);
		gel_list_deep_free(playlist_data, (GFunc) recently_score_record_free, NULL);

		diff_str = recently_calculate_time_diff(now, when);
		tmp2 = g_strdup_printf("<b>%s</b>\n\t%s", diff_str, tmp); 
		g_free(diff_str); g_free(tmp);

		gtk_list_store_append(self->model, &iter);
		gtk_list_store_set(self->model, &iter,
			RECENTLY_COLUMN_TITLE, tmp2,
			RECENTLY_COLUMN_PLAYLIST_FILE, pl_file_basename,
			-1);
		g_free(pl_file_basename);
		g_free(tmp2);

		l = l->next;
	}
	gel_list_deep_free(recent_playlists, (GFunc) g_free, NULL);
}

void
on_recently_dock_row_activated(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	EinaPlugin *plugin)
{
	RecentlyData *self = EINA_PLUGIN_DATA(plugin);
	GtkTreeIter iter;
	gchar *pl_file, *pl_path;
	// void (*callback);
	gchar *buffer = NULL;
	gchar **uris = NULL;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(self->model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(self->model), &iter,
		RECENTLY_COLUMN_PLAYLIST_FILE, &pl_file,
		-1);

	// pl_path = eina_iface_build_plugin_filename(PLUGIN_NAME, pl_file);
	pl_path = eina_plugin_build_userdir_path(plugin, pl_file);

	if (!g_file_get_contents(pl_path, &buffer, NULL, NULL))
	{
		eina_iface_error("Cannot read playlist: '%s'", pl_path);
		g_free(pl_path);
		return;
	}
	g_free(pl_path);

	uris = g_uri_list_extract_uris((const gchar*) buffer);
	g_free(buffer);

	if (uris == NULL)
		return;

	// Delete playlist and meta from disk.
	// recently_playlist_unlink(pl_file);
	recently_playlist_unlink(plugin, pl_file);
	g_free(pl_file);

	// Do a clear, which also saves current playlist
	lomo_player_clear(EINA_PLUGIN_LOMO(plugin));

	// Add uris to lomo. playlist widget is still in background
	// lomo_player_add_uri_multi(EINA_PLUGIN_LOMO_PLAYER(plugin), uris);
	lomo_player_add_uri_strv(EINA_PLUGIN_LOMO(plugin), uris);

	// Now switch to playlist widget
	eina_plugin_dock_switch_item(plugin, "playlist");

	// Free data
	// gel_list_deep_free(uris, (GFunc) g_free, NULL);
	g_strfreev(uris);
}


/*
 * Callbacks
 */
void
on_recently_lomo_clear(LomoPlayer *lomo, EinaPlugin *plugin)
{
	if (!currently_playlist_store(plugin))
	{
		eina_iface_error("Cannot store current playlist");
		return;
	}

	recently_dock_update(plugin);
}

gboolean
recently_init(EinaPlugin *plugin, GError **error)
{
	gchar *path;
	RecentlyData *self;

	path = eina_plugin_build_userdir_path(plugin, NULL);
	if (!g_file_test(path, G_FILE_TEST_IS_DIR) && !g_mkdir_with_parents(path, 0700))
	{
		eina_iface_error("Cannot create plugin dir %s", path);
		g_free(path);
		return FALSE;
	}
	g_free(path);

	plugin->data = self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);

	// Create dock widgets
	self->dock_widget = recently_dock_new(plugin);
	self->dock_label  = gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);

	gtk_widget_show_all(GTK_WIDGET(self->dock_widget));
	gtk_widget_show_all(GTK_WIDGET(self->dock_label));

	eina_plugin_dock_add_item(plugin, "recently",
		self->dock_label,
		self->dock_widget);

	// Setup signals
	eina_plugin_attach_events(plugin,
		"clear", on_recently_lomo_clear,
		NULL);

	return TRUE;
}

gboolean
recently_exit(EinaPlugin *plugin, GError **error)
{
	eina_plugin_dock_remove_item(plugin, "recently");
    eina_plugin_attach_events(plugin,
		"clear", on_recently_lomo_clear,
		NULL);

	return TRUE;
}

G_MODULE_EXPORT EinaPlugin recently_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("Recently"),
	N_("0.2.0"),
	N_("Recently playlists"),
	N_("Recently saves your previously listen playlists"),
	NULL,
	N_("xuzo <xuzo@cuarentaydos.com>"),
	N_("http://eina.sourceforge.net/"),

	recently_init,
	recently_exit,

	NULL, NULL
};
