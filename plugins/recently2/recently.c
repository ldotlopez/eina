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

	// Playlists
	GtkTreeView  *pls_tv;
	GtkListStore *pls_model;

	GtkNotebook *tabs;

	GtkEntry           *search_entry;    
	GtkLabel           *search_tip;
	GtkIconView        *search_view;

	GtkListStore       *queryer_results;
	GtkTreeModelFilter *queryer_filter;

	guint queryer_search_id;
} Recently;

// Model for playlists
enum {
	RECENTLY_COLUMN_TIMESTAMP, // Not visible, for reference
	RECENTLY_COLUMN_SEARCH,    // Not visible, for matching
	RECENTLY_COLUMN_COVER,
	RECENTLY_COLUMN_SUMMARY,
	RECENTLY_COLUMN_MARKUP,
	RECENTLY_COLUMN_PLAY,
	RECENTLY_COLUMN_ENQUEUE,
	RECENTLY_COLUMN_DELETE,

	RECENTLY_N_COLUMNS
};

// Match type for search results
enum {
	MATCH_TYPE_URI    = 0,
	MATCH_TYPE_ARTIST,
	MATCH_TYPE_ALBUM,
	MATCH_TYPE_TITLE,
	MATCH_N_TYPES
};

// Model for search results
enum {
	QUERYER_COLUMN_SEARCH,     // (gpointer)    Store cover search pointer
	QUERYER_COLUMN_MATCH_TYPE, // (gint)        Type of the match
	QUERYER_COLUMN_FULL_MATCH, // (gchar *)     Full match
	QUERYER_COLUMN_COVER,      // (GdkPixbuf *) Artwork 
	QUERYER_COLUMN_TEXT,       // (gchar *)     Text

	QUERYER_N_COLUMNS
};

// Error codes 
enum {
	RECENTLY_NO_ERROR = 0,
	RECENTLY_ERROR_CANNOT_LOAD_ADB,
	RECENTLY_ERROR_CANNOT_UNLOAD_ADB,
	RECENTLY_ERROR_CANNOT_FETCH_ADB
};

// --
// Utils
// --
static GtkWidget *
dock_create(Recently *self);
static void
dock_update(Recently *self);
static gboolean
dock_get_iter_for_search(Recently *self, ArtSearch *search, GtkTreeIter *iter);
static void
dock_update_cover(Recently *self, GtkTreeIter *iter, GdkPixbuf *pixbuf);
static const gchar*
stamp_to_human(gchar *stamp);
static gchar *
summary_playlist(Adb *adb, gchar *timestamp, guint how_many);
static LomoStream*
stream_from_timestamp(Adb *adb, gchar *timestamp);

// --
// Callbacks
// --
static void
dock_row_activated_cb(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	Recently *self);
static void
dock_renderer_edited_cb(GtkWidget *w,
	gchar *path,
	gchar *new_text,
	Recently *self);
static void 
dock_search_entry_changed_cb(Recently *self, GtkEntry *w); // swapped
static void
art_search_success_cb(Art *art, ArtSearch *search, Recently *self);
static void
art_search_fail_cb(Art *art, ArtSearch *search, Recently *self);
static void
lomo_clear_cb(LomoPlayer *lomo, Recently *self);

// --
// The queryer
// --
static void
queryer_schedule_search(Recently *self);
static void
queryer_load_query(Recently *self, gint matchtype, gchar *fullmatch);
static void
queryer_search_view_selected_cb(GtkIconView *search_view, GtkTreePath *arg1, Recently *self);
static void
queryer_search_art_success_cb(Art *art, ArtSearch *search, Recently *self);
static void
queryer_search_art_fail_cb(Art *art, ArtSearch *search, Recently *self);


// --
// ADB related
// --


// --
// ADB
// --
gboolean
upgrade_adb_0(Adb *adb, Recently *self, GError **error);

GQuark recently_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("recently");
	return ret;
}

// --
// API / Utils
// --
static GtkWidget *
dock_create(Recently *self)
{
	// Get GtkBuilder interface
	gchar *xml_path = gel_plugin_build_resource_path(self->plugin, "dock.ui");
	GtkBuilder *xml_ui = gtk_builder_new();

	GError *err = NULL;
	if (gtk_builder_add_from_file(xml_ui, xml_path, &err) == 0)
	{
		gel_error("Cannot load ui from %s: %s", xml_path, err->message);
		g_error_free(err);
		g_object_unref(xml_ui);
		g_free(xml_path);
		return NULL;
	}
	g_free(xml_path);

	GtkWidget *ret = GTK_WIDGET(gtk_builder_get_object(xml_ui, "main-container"));

	g_object_ref(ret);
	gtk_container_remove(
		GTK_CONTAINER(gtk_builder_get_object(xml_ui, "main-window")),
		ret);

	self->pls_tv = GTK_TREE_VIEW(gtk_builder_get_object(xml_ui, "recent-treeview"));

	// Renders
	GtkCellRenderer *renders[RECENTLY_N_COLUMNS];
	renders[RECENTLY_COLUMN_TIMESTAMP] = gtk_cell_renderer_text_new();
	renders[RECENTLY_COLUMN_COVER]     = gtk_cell_renderer_pixbuf_new();
	renders[RECENTLY_COLUMN_MARKUP]    = gtk_cell_renderer_text_new();
	renders[RECENTLY_COLUMN_PLAY]      = gtk_cell_renderer_pixbuf_new();
	renders[RECENTLY_COLUMN_ENQUEUE]   = gtk_cell_renderer_pixbuf_new();
	renders[RECENTLY_COLUMN_DELETE]    = gtk_cell_renderer_pixbuf_new();

	// Columns
	GtkTreeViewColumn *columns[RECENTLY_N_COLUMNS];
	columns[RECENTLY_COLUMN_TIMESTAMP] = gtk_tree_view_column_new_with_attributes(N_("Timestamp"),
		renders[RECENTLY_COLUMN_TIMESTAMP],
		"text", RECENTLY_COLUMN_TIMESTAMP,
		NULL);

	columns[RECENTLY_COLUMN_SEARCH] = NULL;

	columns[RECENTLY_COLUMN_COVER] = gtk_tree_view_column_new_with_attributes(N_("Cover"),
		renders[RECENTLY_COLUMN_COVER],
		"pixbuf", RECENTLY_COLUMN_COVER,
		NULL);

	columns[RECENTLY_COLUMN_SUMMARY] = NULL;

	columns[RECENTLY_COLUMN_MARKUP] = gtk_tree_view_column_new_with_attributes(N_("Title"),
		renders[RECENTLY_COLUMN_MARKUP],
		"markup", RECENTLY_COLUMN_MARKUP,
		NULL);

	columns[RECENTLY_COLUMN_PLAY] = gtk_tree_view_column_new_with_attributes(N_("Play"),
		renders[RECENTLY_COLUMN_PLAY],
		"stock-id", RECENTLY_COLUMN_PLAY,
		NULL);
	columns[RECENTLY_COLUMN_ENQUEUE] = gtk_tree_view_column_new_with_attributes(N_("Enqueue"),
		renders[RECENTLY_COLUMN_ENQUEUE],
		"stock-id", RECENTLY_COLUMN_ENQUEUE,
		NULL);
	columns[RECENTLY_COLUMN_DELETE] = gtk_tree_view_column_new_with_attributes(N_("Delete"),
		renders[RECENTLY_COLUMN_DELETE],
		"stock-id", RECENTLY_COLUMN_DELETE,
		NULL);

	// Add to GtkTreeView
	guint i;
	for (i = 0; i < RECENTLY_N_COLUMNS; i++)
	{
		if (!columns[i])
			continue;
		gtk_tree_view_append_column(self->pls_tv, columns[i]);
		g_object_set(G_OBJECT(columns[i]),
			"visible", i != RECENTLY_COLUMN_TIMESTAMP,
			"resizable", i == RECENTLY_COLUMN_MARKUP,
			"expand", i == RECENTLY_COLUMN_MARKUP,
			NULL);
	}

	// Renderers props
	g_object_set(G_OBJECT(renders[RECENTLY_COLUMN_MARKUP]),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_END,
		"editable", TRUE,
		NULL);

	g_object_set(G_OBJECT(columns[RECENTLY_COLUMN_COVER]),
		"min-width", 32,
		"max-width", 32,
		NULL);

	// Treeview props
    g_object_set(G_OBJECT(self->pls_tv),
		"search-column", -1,
		"headers-clickable", FALSE,
		"headers-visible", FALSE,
		NULL);

	self->pls_model = gtk_list_store_new(RECENTLY_N_COLUMNS,
		G_TYPE_STRING,
		G_TYPE_POINTER,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING);
    gtk_tree_view_set_model(self->pls_tv, GTK_TREE_MODEL(self->pls_model));

	g_signal_connect(self->pls_tv, "row-activated", G_CALLBACK(dock_row_activated_cb), self); 
	g_signal_connect(renders[RECENTLY_COLUMN_MARKUP], "edited", G_CALLBACK(dock_renderer_edited_cb), self);

	// --
	// Setup queryer
	// --
	self->queryer_results = gtk_list_store_new(QUERYER_N_COLUMNS,
		G_TYPE_POINTER,
		G_TYPE_INT,
		G_TYPE_STRING,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING
		);

	self->search_view = GTK_ICON_VIEW(gtk_builder_get_object(xml_ui, "search-iconview"));
	g_object_set(G_OBJECT(self->search_view),
		"pixbuf-column", QUERYER_COLUMN_COVER,
		"markup-column",   QUERYER_COLUMN_TEXT,
		"item-width", 256,
		"columns", -1,
		"selection-mode", GTK_SELECTION_MULTIPLE,
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		"model", self->queryer_results,
		NULL);
	g_signal_connect(self->search_view, "item-activated", (GCallback) queryer_search_view_selected_cb, self);
	// g_signal_connect(self->search_view, "select-cursor-item", (GCallback) queryer_search_view_selected_cb, self);

	self->search_entry = GTK_ENTRY(gtk_builder_get_object(xml_ui, "search-entry")); 
	self->search_tip   = GTK_LABEL(gtk_builder_get_object(xml_ui, "search-tip-label"));
	g_signal_connect_swapped(self->search_entry, "changed", (GCallback) dock_search_entry_changed_cb, self);

	self->tabs = GTK_NOTEBOOK(gtk_builder_get_object(xml_ui, "notebook"));
	gtk_notebook_set_current_page(self->tabs, 0);

	g_object_unref(xml_ui);
	gtk_widget_show_all((GtkWidget *) ret);
	g_idle_add((GSourceFunc) dock_update, self);

	return ret;
}

static void
dock_update(Recently *self)
{
	gtk_list_store_clear(GTK_LIST_STORE(self->pls_model));

	Adb *adb = (Adb*) gel_app_shared_get(self->app, "adb");
	if (adb == NULL)
		return;

	char *q = "SELECT DISTINCT(timestamp) FROM playlist_history WHERE timestamp > 0 ORDER BY timestamp DESC;";
	sqlite3_stmt *stmt = NULL;
	int code = sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL);
	if (code != SQLITE_OK)
	{
		gel_error("Cannot fetch playlist_history data: %d", code);
		return;
	}

	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		gchar *ts = (gchar *) sqlite3_column_text(stmt, 0);
		gchar *title = NULL;

		// Check for alias
		q = sqlite3_mprintf("SELECT alias FROM playlist_aliases where timestamp='%q'", ts);
		sqlite3_stmt *stmt_alias = NULL;
		if (sqlite3_prepare_v2(adb->db, q, -1, &stmt_alias, NULL) == SQLITE_OK)
		{
			if (stmt_alias && (sqlite3_step(stmt_alias) == SQLITE_ROW))
				title = g_strdup((gchar*) sqlite3_column_text(stmt_alias, 0));
			sqlite3_finalize(stmt_alias);
		}
		sqlite3_free(q);

		// If not alias, get summary
		if (title == NULL)
		{
			gchar *summary = summary_playlist(adb, ts, 3);
			title = g_markup_escape_text(summary, -1);
			g_free(summary);
		}

		GtkTreeIter iter;
		gtk_list_store_append((GtkListStore *) self->pls_model, &iter);

		gchar *markup = g_strdup_printf("<b>%s:</b>\n\t%s ", stamp_to_human(ts), title);

		// Start a search for cover
		ArtSearch *search = NULL;
		LomoStream *fake_stream = stream_from_timestamp(adb, ts);
		if (fake_stream)
			search = art_search(GEL_APP_GET_ART(self->app), fake_stream, 
				(ArtFunc) art_search_success_cb, (ArtFunc) art_search_fail_cb,
				self);

		gtk_list_store_set((GtkListStore *) self->pls_model, &iter,
			RECENTLY_COLUMN_TIMESTAMP, ts,
			RECENTLY_COLUMN_SEARCH, search,
			RECENTLY_COLUMN_PLAY, GTK_STOCK_MEDIA_PLAY,
			RECENTLY_COLUMN_ENQUEUE, "eina-queue",
			RECENTLY_COLUMN_DELETE, GTK_STOCK_DELETE,
			RECENTLY_COLUMN_SUMMARY, title,
			RECENTLY_COLUMN_MARKUP, markup,
			-1);
		g_free(title);
		g_free(markup);
	}
	sqlite3_finalize(stmt);
}

static gboolean
dock_get_iter_for_search(Recently *self, ArtSearch *search, GtkTreeIter *iter)
{
	GtkTreeModel *model = gtk_tree_view_get_model(self->pls_tv);

	if (!gtk_tree_model_get_iter_first(model, iter))
	{
		gel_error("Cannot get first iter for model");
		return FALSE;
	}
	do
	{
		ArtSearch *test = NULL;
		gtk_tree_model_get(model, iter,
			RECENTLY_COLUMN_SEARCH, &test, -1);
		if (test == search)
			return TRUE;
	} while (gtk_list_store_iter_is_valid((GtkListStore *) model, iter) && gtk_tree_model_iter_next(model, iter));

	gel_error("Unable to find matching row for search");
	return FALSE;
}

static void
dock_update_cover(Recently *self, GtkTreeIter *iter, GdkPixbuf *pixbuf)
{
	g_return_if_fail(gtk_list_store_iter_is_valid((GtkListStore *) self->pls_model, iter));
	g_return_if_fail(pixbuf);

	gtk_list_store_set(self->pls_model, iter,
		RECENTLY_COLUMN_COVER, gdk_pixbuf_scale_simple(pixbuf, 32, 32, GDK_INTERP_BILINEAR),
		-1);
}

static LomoStream*
stream_from_timestamp(Adb *adb, gchar *timestamp)
{
	char *q = sqlite3_mprintf("SELECT uri,key,value FROM streams JOIN metadata USING(sid) WHERE "
	"sid = (SELECT sid FROM playlist_history WHERE timestamp = '%q' ORDER BY random() LIMIT 1)"
	"AND KEY IN ('album','title','artist')",
	timestamp);
	
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot select a fake stream using query %s", q);
		sqlite3_free(q);
		return NULL;
	}

	gchar *uri = NULL, *title = NULL, *artist = NULL, *album = NULL;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		if (uri == NULL)
			uri = g_strdup((gchar *) sqlite3_column_text(stmt, 0));
		gchar *key   = (gchar *) sqlite3_column_text(stmt, 1);
		gchar *value = (gchar *) sqlite3_column_text(stmt, 2);
		if (g_str_equal(key, "title"))
			title = g_strdup(value);
		else if (g_str_equal(key, "album"))
			album = g_strdup(value);
		else if (g_str_equal(key, "artist"))
			artist = g_strdup(value);
	}
	sqlite3_finalize(stmt);

	if (!artist && !album && !title)
		return NULL;
	
	LomoStream *ret = lomo_stream_new(uri);
	g_object_set_data_full(G_OBJECT(ret), "artist", artist, g_free);
	g_object_set_data_full(G_OBJECT(ret), "title", title, g_free);
	g_object_set_data_full(G_OBJECT(ret), "album", album, g_free);

	return (LomoStream*) ret;
}

// Convert a string in iso8601 format to a more human form
static const gchar*
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

// --
// Dock related
// --
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
	
	gtk_tree_model_get_iter(GTK_TREE_MODEL(self->pls_model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(self->pls_model), &iter,
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
	lomo_player_append_uri_multi(lomo, pl);
	gel_list_deep_free(pl, g_free);

	eina_plugin_switch_dock_widget(self->plugin, "playlist");
	dock_update(self);

	lomo_player_play(lomo, NULL);
}

void
dock_renderer_edited_cb(GtkWidget *w,
	gchar *path,
	gchar *new_text,
	Recently *self)
{
	// If Adb is not present changes could not be saved, so abort
	Adb *adb = GEL_APP_GET_ADB(self->app);
	if (adb == NULL)
	{
		gel_error("Adb not present, reverting edit");
		return;
	}

	// Try to get data from model
	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->pls_model), &iter, path))
	{
		gel_warn("Cannot get iter for path %s", path);
		return;
	}

	gchar *ts = NULL;
	gtk_tree_model_get(GTK_TREE_MODEL(self->pls_model), &iter,
		RECENTLY_COLUMN_TIMESTAMP, &ts,
		-1);

	// Analize input to guess the real intention of the user
	const gchar *ts_human = stamp_to_human(ts);
	gchar *prefix = g_strdup_printf("%s:\n\t", ts_human);
	gchar *alias = NULL;
	if (strstr(new_text, prefix))
		alias = new_text + (strlen(prefix) * sizeof(gchar));
	else
		alias = new_text;

	char *q = NULL;
	char *err = NULL;

	// If alias is NULL or '' delete alias and revert markup to summary
	if ((alias == NULL) || g_str_equal(alias, ""))
	{
		q = sqlite3_mprintf("DELETE FROM playlist_aliases WHERE timestamp='%q'", ts);
		alias = summary_playlist(adb, ts, 3);
	}
	else
	{
		q = sqlite3_mprintf("INSERT OR REPLACE INTO playlist_aliases VALUES('%q', '%q')", ts, alias);
		alias = g_strdup(alias);
	}

	// Update DB
	if (sqlite3_exec(adb->db, q, NULL, NULL, &err) != SQLITE_OK)
	{
		gel_error("Cannot delete alias for %s: %s", ts, err);
		sqlite3_free(err);
	}
	sqlite3_free(q);

	gchar *real_new_text = g_strdup_printf("<b>%s</b>:\n\t\%s",
		ts_human,
		alias);
	gtk_list_store_set(GTK_LIST_STORE(self->pls_model), &iter,
		RECENTLY_COLUMN_MARKUP, real_new_text,
		-1);
	g_free(real_new_text);
	g_free(prefix);
	g_free(alias);
}

static void
dock_search_entry_changed_cb(Recently *self, GtkEntry *w)
{
	const gchar *text = gtk_entry_get_text(self->search_entry);
	gssize len = g_utf8_strlen(text, -1);

	if (len < 3)
	{
		gtk_notebook_set_current_page(self->tabs, 0);
		if (!GTK_WIDGET_VISIBLE(self->search_tip))
			gtk_widget_show((GtkWidget *) self->search_tip);
	}
	else
	{
		if (GTK_WIDGET_VISIBLE(self->search_tip))
			gtk_widget_hide((GtkWidget *) self->search_tip);
		queryer_schedule_search(self);
	}
	/*
	if (len 
	gel_warn("Current text: %s",  gtk_entry_get_text(self->search_entry));
	 */
}

static void
search_get_sql_results(Recently *self, gchar *input)
{
	// Too complex search, since this is a prototype we start with a simplier
	// approach
#if 0
	GString *like = g_string_sized_new(g_utf8_strlen(input, -1) * 2);
	while (input && input[0])
	{
		like = g_string_append_c(like, '%');
		like = g_string_append_unichar(like,  g_utf8_get_char(input));
		input = g_utf8_find_next_char(input, NULL);
	}
	like = g_string_append_c(like, '%');
	gchar *normalized = g_utf8_normalize(like->str, -1, G_NORMALIZE_NFKD);
	g_string_free(like, TRUE);
#endif

	Adb *adb = GEL_APP_GET_ADB(self->app);
	Art *art = GEL_APP_GET_ART(self->app);

	char *q = sqlite3_mprintf("SELECT sid,uri,artist,album,title FROM recently_flat_metadata WHERE "
		"uri like '%%%q%%' or artist like '%%%q%%' or album like '%%%q%%' or title like '%%%q%%' "
		"GROUP BY album,artist",
		input, input, input, input);
	gel_warn("Search '%s'", q);

	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot query ADB for matches");	
		sqlite3_free(q);
		return;
	}

	gchar *tmp = g_strdup_printf(".*%s.*", input);
	GRegex *regex = g_regex_new(tmp,
		G_REGEX_CASELESS|G_REGEX_DOTALL|G_REGEX_DOLLAR_ENDONLY|G_REGEX_OPTIMIZE|G_REGEX_NO_AUTO_CAPTURE,
		0, NULL);
	g_free(tmp);
	GtkTreeIter iter;
	gtk_list_store_clear(self->queryer_results);
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		gchar *u = (gchar*) sqlite3_column_text(stmt, 1); 
		gchar *a = (gchar*) sqlite3_column_text(stmt, 2); 
		gchar *b = (gchar*) sqlite3_column_text(stmt, 3); 
		gchar *t = (gchar*) sqlite3_column_text(stmt, 4);

		gchar *text = NULL;
		gint match_type = 0;
		gchar *full_match = NULL;
		if (g_regex_match(regex, a, 0, NULL))
		{
			match_type = MATCH_TYPE_ARTIST;
			full_match = a;
			text = g_strdup_printf(N_("<b>Artist:</b>\n  %s"), a);
		}
		else if (g_regex_match(regex,b, 0, NULL ))
		{
			match_type = MATCH_TYPE_ALBUM;
			full_match = b;
			text = g_strdup_printf(N_("<b>Album:</b>\n  %s (by %s)"), b, a);
		}
		else if (g_regex_match(regex,t, 0, NULL))
		{
			match_type = MATCH_TYPE_TITLE;
			full_match = t;
			text = g_strdup_printf(N_("<b>Song:</b>\n  %s (by %s)"), t, a);
		}
		else if (g_regex_match(regex,u, 0, NULL))
		{
			match_type = MATCH_TYPE_URI;
			full_match = u;
			text = g_strdup_printf(N_("<b>File:</b>\n  %s)"),u );
		}

		LomoStream *stream = lomo_stream_new(u);
		g_object_set_data(G_OBJECT(stream), "artist", g_strdup(a));
		g_object_set_data(G_OBJECT(stream), "album",  g_strdup(b));
		g_object_set_data(G_OBJECT(stream), "title",  g_strdup(t));

		ArtSearch *search =  art_search(art, stream,
			(ArtFunc) queryer_search_art_success_cb, (ArtFunc) queryer_search_art_fail_cb,
			self);

		gtk_list_store_append(GTK_LIST_STORE(self->queryer_results), &iter);
		gtk_list_store_set(GTK_LIST_STORE(self->queryer_results), &iter,
			QUERYER_COLUMN_SEARCH, search,
			QUERYER_COLUMN_MATCH_TYPE, match_type,
			QUERYER_COLUMN_FULL_MATCH, g_strdup(full_match),
			QUERYER_COLUMN_TEXT, text,
			-1);
		g_free(text);

		gel_warn("MATCH(%d) [%s]: %s %s %s", match_type, full_match, a , b, t);
	}
	g_regex_unref(regex);
	sqlite3_finalize(stmt);
	sqlite3_free(q);
}

static gboolean
queryer_run_search(Recently *self)
{
	search_get_sql_results(self, (gchar *) gtk_entry_get_text(self->search_entry));
	gtk_notebook_set_current_page(self->tabs, 1);
	self->queryer_search_id = 0;
	return FALSE;
}

static void
queryer_schedule_search(Recently *self)
{
	if (self->queryer_search_id)
		g_source_remove(self->queryer_search_id);
	self->queryer_search_id = g_timeout_add_seconds(1, (GSourceFunc) queryer_run_search, self);
}

static void
queryer_load_query(Recently *self, gint matchtype, gchar *fullmatch)
{
	const gchar *keys[MATCH_N_TYPES] = { "uri", "artist", "album", "title" };

	g_return_if_fail((matchtype >= 0) && (matchtype < MATCH_N_TYPES) && (keys[matchtype] != NULL));

	GList *uris = NULL;
	gchar *q = sqlite3_mprintf("SELECT uri FROM recently_flat_metadata WHERE %q = '%q'", keys[matchtype], fullmatch);
	
	Adb *adb = GEL_APP_GET_ADB(self->app);
	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot prepare query %s", q);
		sqlite3_free(q);
		return;
	}

	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
		uris = g_list_prepend(uris, g_strdup((gchar *) sqlite3_column_text(stmt, 0)));
	sqlite3_finalize(stmt);

	uris = g_list_reverse(uris);
	lomo_player_append_uri_multi(GEL_APP_GET_LOMO(self->app), uris);
	gel_list_deep_free(uris, (GFunc) g_free);
}

static void
queryer_search_view_selected_cb(GtkIconView *search_view, GtkTreePath *arg1, Recently *self)
{
	GList *s = gtk_icon_view_get_selected_items(search_view);

	GtkListStore *model = GTK_LIST_STORE(gtk_icon_view_get_model(search_view));
	GtkTreeIter iter;
	GList *i = s;
	while (i)
	{
		if (!gtk_tree_model_get_iter((GtkTreeModel *) model, &iter, (GtkTreePath *) i->data))
		{
			gchar *pathstr = gtk_tree_path_to_string((GtkTreePath *) i->data);
			gel_warn("Cannot get iter for %s", pathstr);
			g_free(pathstr);
			continue;
		}

		gchar *fullmatch;
		gint    matchtype;
		gtk_tree_model_get((GtkTreeModel *) model, &iter,
			QUERYER_COLUMN_FULL_MATCH, &fullmatch,
			QUERYER_COLUMN_MATCH_TYPE, &matchtype,
			-1);
		queryer_load_query(self, matchtype, fullmatch);
		g_free(fullmatch);

		i = i->next;
	}
	gel_list_deep_free(s,  (GFunc) gtk_tree_path_free);
}

static void
queryer_search_art_success_cb(Art *art, ArtSearch *search, Recently *self)
{
	GtkTreeIter iter;
	gpointer test;

	gtk_tree_model_get_iter_first((GtkTreeModel *) self->queryer_results, &iter);
	while (gtk_list_store_iter_is_valid(self->queryer_results, &iter))
	{
		gtk_tree_model_get((GtkTreeModel *) self->queryer_results, &iter,
			QUERYER_COLUMN_SEARCH, &test,
			-1);
		if (test == search)
		{
			GdkPixbuf *pb = art_search_get_result(search);
			gtk_list_store_set(self->queryer_results, &iter,
				QUERYER_COLUMN_SEARCH, NULL,
				QUERYER_COLUMN_COVER, gdk_pixbuf_scale_simple(pb, 64, 64, GDK_INTERP_BILINEAR),
				-1);
			g_object_unref(pb);
			gel_warn("Cover set");
			break;
		}
		gtk_tree_model_iter_next((GtkTreeModel *) self->queryer_results, &iter);
	}
	gel_warn("success finish");
	g_object_unref(G_OBJECT(art_search_get_stream(search)));
}

static void
queryer_search_art_fail_cb(Art *art, ArtSearch *search, Recently *self)
{
	gel_warn("Search failed");
	g_object_unref(G_OBJECT(art_search_get_stream(search)));
}

// --
// Callbacks
// --
static void
art_search_success_cb(Art *art, ArtSearch *search, Recently *self)
{
	GtkTreeIter iter;

	if (dock_get_iter_for_search(self, search, &iter))
		dock_update_cover(self, &iter, art_search_get_result(search));
	g_object_unref(art_search_get_stream(search));
}

static void
art_search_fail_cb(Art *art, ArtSearch *search, Recently *self)
{
	LomoStream *stream = art_search_get_stream(search);
	gel_implement("Set unknow cover on failure");
	g_object_unref(stream);
}
static void
lomo_clear_cb(LomoPlayer *lomo, Recently *self)
{
	dock_update(self);
}

// --
// ADB
// --
gboolean
upgrade_adb_0(Adb *adb, Recently *self, GError **error)
{
	gchar *q[] = {
		"DROP TABLE IF EXISTS playlist_aliases;",

		"CREATE TABLE IF NOT EXISTS playlist_aliases ("
		"	timestamp TIMESTAMP PRIMARY KEY UNIQUE,"
		"	alias VARCHAR(128),"
		"	FOREIGN KEY(timestamp) REFERENCES playlist_history(timestamp) ON DELETE CASCADE ON UPDATE RESTRICT"
		");",

		NULL
		};
	return adb_exec_queryes(adb, q, NULL, error);
}

gboolean
recently_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	// Load adb, upgrade database
	GError *err = NULL;
	if (!gel_app_load_plugin_by_name(app, "adb", &err))
	{
		g_set_error(error, recently_quark(), RECENTLY_ERROR_CANNOT_LOAD_ADB, 
			N_("Cannot load adb plugin: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}

	// Upgrade database
	Adb *adb;
	if ((adb = GEL_APP_GET_ADB(app)) == NULL)
	{
		g_set_error(error, recently_quark(), RECENTLY_ERROR_CANNOT_FETCH_ADB, N_("Cannot fetch Adb object"));
		gel_app_unload_plugin_by_name(app, "adb", NULL);
		return FALSE;
	}

	gpointer upgrades[] = {
		upgrade_adb_0, NULL
	};
	if (!adb_schema_upgrade(adb, "recently", upgrades, NULL, error))
		return FALSE;

	// Create a view
	gchar *view_query = "DROP VIEW IF EXISTS recently_flat_metadata;"
	"CREATE TEMP VIEW recently_flat_metadata AS SELECT streams.sid as sid,uri,artist,album,title FROM "
	"   (SELECT sid, uri   AS uri    FROM streams)                       AS streams JOIN "
	"	(SELECT sid, value AS artist FROM metadata where key = 'artist') AS artists JOIN "
	"	(SELECT sid, value AS album  FROM metadata where key = 'album')  AS albums JOIN "
	"	(SELECT sid, value AS title  FROM metadata where key = 'title')  AS titles "
	"ON streams.sid = artists.sid AND artists.sid = albums.sid AND albums.sid = titles.sid;";
	gchar *sql_err = NULL;
	if (sqlite3_exec(adb->db, view_query, NULL, NULL, &sql_err) != SQLITE_OK)
	{
		gel_error("Cannot create view: %s", sql_err);
		sqlite3_free(sql_err);
	}

	// Create dock
	Recently *self = g_new0(Recently, 1);
	self->app    = app;
	self->plugin = plugin;
	self->dock   = dock_create(self);
	eina_plugin_add_dock_widget(plugin, "recently", gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU), self->dock);

	// Signals
	g_signal_connect(GEL_APP_GET_LOMO(app), "clear", (GCallback) lomo_clear_cb, self);

	plugin->data = self;
	return TRUE;
}

gboolean
recently_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	GError *err = NULL;
	eina_plugin_remove_dock_widget(plugin, "recently");

	Adb *adb = GEL_APP_GET_ADB(app);
	gchar *sql_err = NULL;
	if (!sqlite3_exec(adb->db, "DROP VIEW IF EXISTS recently_flat_metadata", NULL, NULL, &sql_err) != SQLITE_OK)
	{
		gel_warn("Cannot drop view: %s", sql_err);
		sqlite3_free(sql_err);
	}

	if (!gel_app_unload_plugin_by_name(app, "adb", &err))
	{
		g_set_error(error, recently_quark(), RECENTLY_ERROR_CANNOT_UNLOAD_ADB, 
			N_("Cannot unload adb plugin: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}
	g_free(plugin->data);
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
