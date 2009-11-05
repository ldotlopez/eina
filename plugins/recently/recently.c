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

#include <config.h>

#define GEL_DOMAIN "Eina::Plugin::Recently"
#define EINA_PLUGIN_DATA_TYPE "Recently"
#define EINA_PLUGIN_NAME "Recently"

#include <string.h>
#include <eina/eina-plugin.h>
#include <plugins/adb/adb.h>

typedef struct {
	GelApp       *app;
	GelPlugin    *plugin;

	// Dock
	GtkWidget    *dock;
	GtkNotebook *tabs;

	// Recently
	GtkTreeView  *pls_tv;
	GtkListStore *pls_model;

	// Queryer
	GtkEntry           *search_entry;    
	GtkLabel           *search_tip;
	GtkIconView        *search_view;

	GtkListStore       *queryer_results;
	GtkTreeModelFilter *queryer_filter;

} Recently;

// Model for playlist 
enum {
	RECENTLY_COLUMN_TIMESTAMP, // (gchar *)     Timestamp of the playlist
	RECENTLY_COLUMN_SEARCH,    // (gpointer)    Search for cover
	RECENTLY_COLUMN_COVER,     // (GdkPixbuf *) Cover
	RECENTLY_COLUMN_MARKUP,    // (gchar*)      Markup to render
	RECENTLY_COLUMN_PLAY,      // (GdkPixbuf *) Play indicator
	RECENTLY_COLUMN_ENQUEUE,   // (GdkPixbuf *) Enqueue indicator
	RECENTLY_COLUMN_DELETE,    // (GdkPixbuf *) Delete indicator

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

enum {
	QUERYER_STATE_NULL = 0,
	QUERYER_STATE_SQL,
	QUERYER_STATE_REFINE
};

enum {
	TAB_RECENTLY = 0,
	TAB_QUERYER,
	TAB_INFO
};

// Error codes 
enum {
	RECENTLY_NO_ERROR = 0,
	RECENTLY_ERROR_CANNOT_LOAD_ADB,
	RECENTLY_ERROR_CANNOT_UNLOAD_ADB,
	RECENTLY_ERROR_CANNOT_FETCH_ADB
};

#define RECENTLY_MARKUP "<b>%s:</b>\n\t%s"
#define RECENTLY_COVER_SIZE 32
#define QUERYER_COVER_SIZE 64

// --
// Generic stuff
// --
static gboolean
list_store_get_iter_for_search(GtkListStore *model, gint column, ArtSearch *search, GtkTreeIter *iter);
static void
list_store_set_cover(GtkListStore *model, gint column, GtkTreeIter *iter, GdkPixbuf *pixbuf, guint size);
static const gchar*
stamp_to_human(gchar *stamp);

// --
// Dock related
// --
static GtkWidget*
dock_create(Recently *self);
static void 
dock_search_entry_changed_cb(Recently *self, GtkEntry *w); // swapped

// --
// Recently tab
// --
static gboolean
recently_refresh(Recently *self);
static void
recently_row_activated_cb(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
Recently *self);
static void
recently_markup_edited_cb(GtkWidget *w,
	gchar *path,
	gchar *new_text,
	Recently *self);
static void
recently_search_cb(Art *art, ArtSearch *search, Recently *self);

// --
// The queryer
// --
static void
queryer_fill_model_for_query(Recently *self, gchar *input);
static void
queryer_load_query(Recently *self, gint matchtype, gchar *fullmatch);
static void
queryer_search_view_selected_cb(GtkIconView *search_view, GtkTreePath *arg1, Recently *self);
static void
queryer_search_cb(Art *art, ArtSearch *search, Recently *self);
static gboolean
queryer_tree_model_filter_visible_cb(GtkTreeModel *model, GtkTreeIter *iter, Recently *self);

// --
// ADB utils
// --
gboolean
adb_upgrade_0(Adb *adb, Recently *self, GError **error);
static gchar *
adb_get_summary_from_timestamp(Adb *adb, gchar *timestamp, guint how_many);
static LomoStream*
adb_get_stream_from_timestamp(Adb *adb, gchar *timestamp);
static gchar*
adb_get_title_for_timestamp(Adb *adb, gchar *timestamp);
static gchar**
adb_get_n_timestamps(Adb *adb, gint n);
static gchar**
adb_get_playlist_from_timestamp(Adb *adb, gchar *timestamp);
static void
adb_delete_playlist_from_timestamp(Adb *Adb, gchar *timestamp);
static LomoStream*
adb_get_stream_from_uri(Adb *adb, gchar *uri);

// --
// Lomo callbacks
// --
static void
lomo_clear_cb(LomoPlayer *lomo, Recently *self);

// --
// --
// Implementation
// --
// --

GQuark recently_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("recently");
	return ret;
}

// --
// Generic stuff
// --
static gboolean
list_store_get_iter_for_search(GtkListStore *model, gint column, ArtSearch *search, GtkTreeIter *iter)
{
	g_return_val_if_fail(gtk_tree_model_get_iter_first((GtkTreeModel *) model, iter), FALSE);

	do
	{
		ArtSearch *test = NULL;
		gtk_tree_model_get((GtkTreeModel *) model, iter, column, &test, -1);
		if (test == search)
			return TRUE;
	} while (gtk_list_store_iter_is_valid(model, iter) && gtk_tree_model_iter_next((GtkTreeModel *) model, iter));

	gel_error(N_("Unable to find matching row for search"));
	return FALSE;
}

static void
list_store_set_cover(GtkListStore *model, gint column, GtkTreeIter *iter, GdkPixbuf *pixbuf, guint size)
{
	g_return_if_fail(gtk_list_store_iter_is_valid(model, iter));
	g_return_if_fail(size > 0);

	if (pixbuf == NULL)
	{
		// gchar *path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-default.png");
		gchar *path = gel_plugin_get_resource(NULL, GEL_RESOURCE_IMAGE, "cover-default.png");
		if (!path)
		{
			gel_error(N_("Cannot get resource cover-default.png"));
			return;
		}

		GError *err = NULL;
		pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &err);
		if (!pixbuf)
		{
			gel_error(N_("Cannot load pixbuf from %s: %s"), path, err->message);
			g_error_free(err);
			g_free(path);
			return;
		}
		g_free(path);
	}

	gtk_list_store_set(model, iter, column, gdk_pixbuf_scale_simple(pixbuf, size, size, GDK_INTERP_BILINEAR), -1);
	g_object_unref(pixbuf);
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

// --
// Dock related
// --
static GtkWidget *
dock_create(Recently *self)
{
	// Get GtkBuilder interface
	// gchar *xml_path = gel_plugin_build_resource_path(self->plugin, "dock.ui");
	gchar *xml_path = gel_plugin_get_resource(self->plugin, GEL_RESOURCE_UI, "dock.ui");
	GtkBuilder *xml_ui = gtk_builder_new();

	GError *err = NULL;
	if (gtk_builder_add_from_file(xml_ui, xml_path, &err) == 0)
	{
		gel_error(N_("Cannot load ui from %s: %s"), xml_path, err->message);
		g_error_free(err);
		g_object_unref(xml_ui);
		g_free(xml_path);
		return NULL;
	}
	g_free(xml_path);

	// Get main widget
	GtkWidget *ret = GTK_WIDGET(gtk_builder_get_object(xml_ui, "main-container"));
	g_object_ref(ret);
	gtk_container_remove(
		GTK_CONTAINER(gtk_builder_get_object(xml_ui, "main-window")),
		ret);

	// --
	// Recently playlists
	// --

	// Build recently model and his view
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

	// self->pls_model = GTK_LIST_STORE(gtk_tree_view_get_model(self->pls_tv));
	// gel_warn("Model for recently treeview: %p (%p)", gtk_tree_view_get_model(self->pls_tv), self->pls_model);

	g_signal_connect(self->pls_tv, "row-activated", G_CALLBACK(recently_row_activated_cb), self); 
	g_signal_connect(renders[RECENTLY_COLUMN_MARKUP], "edited", G_CALLBACK(recently_markup_edited_cb), self);
	g_idle_add((GSourceFunc) recently_refresh, self);

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
	g_signal_connect(self->search_view, "select-cursor-item", (GCallback) queryer_search_view_selected_cb, self);

	self->search_entry = GTK_ENTRY(gtk_builder_get_object(xml_ui, "search-entry")); 
	self->search_tip   = GTK_LABEL(gtk_builder_get_object(xml_ui, "search-tip-label"));
	g_signal_connect_swapped(self->search_entry, "changed", (GCallback) dock_search_entry_changed_cb, self);

	self->tabs = GTK_NOTEBOOK(gtk_builder_get_object(xml_ui, "notebook"));
	gtk_notebook_set_current_page(self->tabs, 0);
	gtk_icon_view_set_model(self->search_view, NULL);

	g_object_unref(xml_ui);
	gtk_widget_show_all((GtkWidget *) ret);
	gtk_widget_hide((GtkWidget *) self->search_tip);

	return ret;
}

static void
dock_search_entry_changed_cb(Recently *self, GtkEntry *w)
{
	const gchar *q = gtk_entry_get_text(w);
	gssize len = g_utf8_strlen(q, -1);

	// Search tip
	if ((len >= 1) && (len < 3))
	{
		if (!GTK_WIDGET_VISIBLE(self->search_tip))
			gtk_widget_show((GtkWidget *) self->search_tip);
	}
	else
	{
		if (GTK_WIDGET_VISIBLE(self->search_tip))
			gtk_widget_hide((GtkWidget *) self->search_tip);
	}

	// Tab to show
	if (len < 3)
		gtk_notebook_set_current_page(self->tabs, 0);
	else
		gtk_notebook_set_current_page(self->tabs, 1);

	// Fill the search_results model or free it
	if (len < 3)
	{
		gel_warn("Discart all models");
		gtk_list_store_clear(self->queryer_results);
		gel_free_and_invalidate(self->queryer_filter, NULL, g_object_unref);
	}
	else if ((len >= 3) && (gtk_icon_view_get_model(self->search_view) == NULL))
	{
		gel_warn("Fill results model, create filter");

		queryer_fill_model_for_query(self, (gchar *) q);

		gel_free_and_invalidate(self->queryer_filter, NULL, g_object_unref);
		self->queryer_filter = (GtkTreeModelFilter *) gtk_tree_model_filter_new((GtkTreeModel *) self->queryer_results, NULL);
		gtk_tree_model_filter_set_visible_func(self->queryer_filter,
			(GtkTreeModelFilterVisibleFunc) queryer_tree_model_filter_visible_cb,
			self, NULL);
	}

	// Must be refreshed?
	if (len > 3)
		gtk_tree_model_filter_refilter(self->queryer_filter);

	// Which model should be displayed
	if (len < 3)
	{
		gel_warn("Set model for view to null");
		gtk_icon_view_set_model(self->search_view, NULL);
	}

	else if ((len >= 3) && (gtk_icon_view_get_model(self->search_view) == NULL))
	{
		gel_warn("Set model to filter");
		gtk_icon_view_set_model(self->search_view, (GtkTreeModel *) self->queryer_filter);
	}
}

// --
// Recently tab
// --
static gboolean
recently_refresh(Recently *self)
{
	Adb *adb = (Adb*) gel_app_shared_get(self->app, "adb");
	g_return_val_if_fail(adb != NULL, FALSE);

	gtk_list_store_clear(GTK_LIST_STORE(self->pls_model));

	gchar **timestamps = adb_get_n_timestamps(adb, -1);
	gint i;
	for (i = 0; timestamps && timestamps[i]; i++) 
	{
		gchar *title = adb_get_title_for_timestamp(adb, timestamps[i]);
		gchar *escaped = g_markup_escape_text(title, -1);
		g_free(title);
		gchar *markup = g_strdup_printf(N_(RECENTLY_MARKUP), stamp_to_human(timestamps[i]), escaped);
		g_free(escaped);

		// Start a search for cover
		ArtSearch *search = NULL;
		LomoStream *fake_stream = adb_get_stream_from_timestamp(adb, timestamps[i]);
		if (fake_stream)
			search = art_search(GEL_APP_GET_ART(self->app), fake_stream, (ArtFunc) recently_search_cb, self);

		GtkTreeIter iter;
		gtk_list_store_append((GtkListStore *) self->pls_model, &iter);
		gtk_list_store_set((GtkListStore *) self->pls_model, &iter,
			RECENTLY_COLUMN_TIMESTAMP, timestamps[i],
			RECENTLY_COLUMN_SEARCH, search,
			RECENTLY_COLUMN_COVER, NULL,
			RECENTLY_COLUMN_MARKUP, markup, 
			RECENTLY_COLUMN_PLAY, GTK_STOCK_MEDIA_PLAY,
			RECENTLY_COLUMN_ENQUEUE, EINA_STOCK_QUEUE,
			RECENTLY_COLUMN_DELETE, GTK_STOCK_DELETE, 
			-1);
		g_free(markup);
	}

	g_strfreev(timestamps);

	return FALSE;
}

void
recently_row_activated_cb(GtkWidget *w,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	Recently *self)
{
	GtkTreeIter iter;
	gchar *ts;

	Adb *adb = (Adb *) gel_app_shared_get(self->app, "adb");
	LomoPlayer *lomo = (LomoPlayer *) gel_app_shared_get(self->app, "lomo");
	g_return_if_fail((adb != NULL) && (lomo != NULL));

	GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(w));
	gint action = g_list_length(columns) - (g_list_index(columns, column) + 1);
	g_list_free(columns);
	
	gboolean do_play = FALSE;
	gboolean do_delete = FALSE;

	switch (action)
	{
	case 4:
	case 2:
		do_play = TRUE;
		do_delete = TRUE;
		break;
	case 1:
		do_play = TRUE;
		do_delete = FALSE;
		break;
	case 0:
		do_play = FALSE;
		do_delete = TRUE;
		break;
	}
	
	gtk_tree_model_get_iter(GTK_TREE_MODEL(self->pls_model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(self->pls_model), &iter,
		RECENTLY_COLUMN_TIMESTAMP, &ts,
		-1);

	gchar **pl = NULL;

	// Get playlist if we are going to play it
	if (do_play)
		pl = adb_get_playlist_from_timestamp(adb, ts);

	if (do_delete)
		adb_delete_playlist_from_timestamp(adb, ts);

	if (do_delete || do_play)
		gtk_list_store_remove((GtkListStore *) self->pls_model, &iter);

	if (do_delete && do_play)
		lomo_player_clear(lomo);

	if (do_play)
	{
		lomo_player_append_uri_strv(lomo, pl);
		eina_plugin_switch_dock_widget(self->plugin, "playlist");

		lomo_player_play(lomo, NULL);
		g_strfreev(pl);
	}

	g_free(ts);
}

void
recently_markup_edited_cb(GtkWidget *w,
	gchar *path,
	gchar *new_text,
	Recently *self)
{
	// If Adb is not present changes could not be saved, so abort
	Adb *adb = GEL_APP_GET_ADB(self->app);
	g_return_if_fail(adb != NULL);

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
		alias = adb_get_summary_from_timestamp(adb, ts, 3);
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
recently_search_cb(Art *art, ArtSearch *search, Recently *self)
{
	GtkTreeIter iter;

	if (list_store_get_iter_for_search(self->pls_model, RECENTLY_COLUMN_SEARCH, search, &iter))
		list_store_set_cover(self->pls_model, RECENTLY_COLUMN_COVER, &iter, art_search_get_result(search), RECENTLY_COVER_SIZE);
	g_object_unref(art_search_get_stream(search)); 
}

// --
// ADB
// --
gboolean
adb_upgrade_0(Adb *adb, Recently *self, GError **error)
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

static LomoStream*
adb_get_stream_from_timestamp(Adb *adb, gchar *timestamp)
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

static gchar *
adb_get_summary_from_timestamp(Adb *adb, gchar *timestamp, guint how_many)
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

static gchar*
adb_get_title_for_timestamp(Adb *adb, gchar *timestamp)
{
	gchar *title = NULL;

	char *q = sqlite3_mprintf("SELECT alias FROM playlist_aliases where timestamp='%q'", timestamp);
	sqlite3_stmt *stmt_alias = NULL;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt_alias, NULL) == SQLITE_OK)
	{
		if (stmt_alias && (sqlite3_step(stmt_alias) == SQLITE_ROW))
			title = g_strdup((gchar*) sqlite3_column_text(stmt_alias, 0));
		sqlite3_finalize(stmt_alias);
	}
	sqlite3_free(q);

	if (title != NULL)
		return title;
	else
		return adb_get_summary_from_timestamp(adb, timestamp, 3);
}

static gchar**
adb_get_n_timestamps(Adb *adb, gint n)
{
	g_return_val_if_fail(adb != NULL, NULL);
	n = CLAMP(n, -1, G_MAXINT);

	char *q = sqlite3_mprintf("SELECT DISTINCT(timestamp) FROM playlist_history "
		"WHERE timestamp > 0 "
		"ORDER BY timestamp DESC "
		"LIMIT %d;", n);
	sqlite3_stmt *stmt = NULL;
	int code = sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL);
	if (code != SQLITE_OK)
	{
		gel_error("Cannot fetch playlist_history data: %d", code);
		sqlite3_free(q);
		return NULL;
	}

	GList *tmp = NULL;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		gchar *timestamp = (gchar *) sqlite3_column_text(stmt, 0);
		tmp = g_list_prepend(tmp, g_strdup(timestamp));
	}
	sqlite3_finalize(stmt);
	sqlite3_free(q);

	gchar **ret = gel_list_to_strv(g_list_reverse(tmp), TRUE);
	g_list_free(tmp);
	return ret;
}

static gchar **
adb_get_playlist_from_timestamp(Adb *adb, gchar *timestamp)
{
	char *q = sqlite3_mprintf(
		"SELECT uri FROM streams WHERE sid IN ("
		"SELECT sid FROM playlist_history WHERE timestamp='%q'" 
		") ORDER BY sid DESC;", timestamp);
	sqlite3_stmt *stmt = NULL;
	int code = sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL);
	if (code != SQLITE_OK)
	{
		gel_warn(N_("Error %d with query %s: %s"), code, q, sqlite3_errmsg(adb->db));
		sqlite3_free(q);
		return NULL;
	}

	GList *pl = NULL;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
		pl = g_list_prepend(pl, g_strdup((gchar*) sqlite3_column_text(stmt, 0)));
	sqlite3_finalize(stmt);
	sqlite3_free(q);

	gchar **ret = gel_list_to_strv(pl, FALSE);
	g_list_free(pl);
	return ret;
}

static void
adb_delete_playlist_from_timestamp(Adb *adb, gchar *timestamp)
{
	char *q = sqlite3_mprintf("DELETE FROM playlist_history WHERE timestamp='%q'", timestamp);
	char *err = NULL;
	if (sqlite3_exec(adb->db, q, NULL, NULL, &err) != SQLITE_OK)
	{
		gel_error("Cannot delete playlist %s form history: %s", timestamp, err);
		sqlite3_free(err);
	}
	sqlite3_free(q);
}

static LomoStream*
adb_get_stream_from_uri(Adb *adb, gchar *uri)
{
	char *q = sqlite3_mprintf(
		"SELECT key,value FROM metadata "
		"WHERE sid = (SELECT sid FROM streams WHERE uri = '%q');", uri);

	sqlite3_stmt *stmt = NULL;
	int code = sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL);
	if (code != SQLITE_OK)
	{
		gel_error(N_("Cannot prepare query %s: (%d) %s"), q, code, sqlite3_errmsg(adb->db));
		sqlite3_free(q);
		return NULL;
	}

	LomoStream *ret = lomo_stream_new(uri);
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		const gchar *k = (const gchar *) sqlite3_column_text(stmt, 0);
		const gchar *v = (const gchar *) sqlite3_column_text(stmt, 1);

		LomoTag tag = LOMO_TAG_INVALID;
		if (g_str_equal(k, "album"))
			tag = LOMO_TAG_ALBUM;
		else if (g_str_equal(k, "artist"))
			tag = LOMO_TAG_ARTIST;
		else if (g_str_equal(k, "title"))
			tag = LOMO_TAG_TITLE;

		if (tag != LOMO_TAG_INVALID)
			lomo_stream_set_tag(ret, tag, g_strdup(v));
	}
	return ret;
}


// --
// Dock related
// --
static void
queryer_fill_model_for_query(Recently *self, gchar *input)
{
	Adb *adb = GEL_APP_GET_ADB(self->app);
	Art *art = GEL_APP_GET_ART(self->app);

	gtk_list_store_clear(self->queryer_results);

	g_return_if_fail(adb != NULL);
	g_return_if_fail(art != NULL);

	char *keys[] = {
	//	"uri",
		"album",
		"artist",
		"title",
		NULL };
	gint match_t[] = {
	//	MATCH_TYPE_URI,
		MATCH_TYPE_ALBUM,
		MATCH_TYPE_ARTIST,
		MATCH_TYPE_TITLE,
		-1 };

	/*
	gchar *markups[] = {
		N_("File"),
		N_("Album"),
		N_("Artist"),
		N_("Title"),
		NULL };
	*/
	char *base_q = "SELECT sid,c,value,uri FROM "
		"(SELECT sid,value,count(*) as c from metadata WHERE "
		"	key='%q' and UPPER(value) LIKE ('%%'||UPPER('%q')||'%%') GROUP BY (value)) "
		"JOIN streams USING(sid);";

	int i = 0;
	for (i = 0; keys[i] != NULL; i++)
	{
		char *q = NULL;
		q = sqlite3_mprintf(base_q, keys[i], input);
		sqlite3_stmt *stmt = NULL;
		if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
		{
			gel_error(N_("Cannot prepare query %s: %s"), q, sqlite3_errmsg(adb->db));
			sqlite3_free(q);
			continue;
		}
		sqlite3_free(q);
		
		while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
		{
			const gchar *fullmatch = (const gchar*) sqlite3_column_text(stmt, 2);
			const gint   n_matches = (const gint  ) sqlite3_column_int (stmt, 1);
			gchar *escaped = g_markup_escape_text(fullmatch, -1);

			gchar *markup = NULL;
			switch (match_t[i])
			{
			case MATCH_TYPE_ARTIST:
				markup = g_strdup_printf(N_("<b>%s (Artist)</b>\n %d matches"), escaped, n_matches);
				break;
			case MATCH_TYPE_ALBUM:
				markup = g_strdup_printf(N_("<b>%s (Album)</b>\n %d matches"), escaped, n_matches);
				break;
			case MATCH_TYPE_TITLE:
				markup = g_strdup_printf(N_("<b>%s</b>\n"), escaped);
				break;
			default:
				markup = g_strdup(escaped);

			}
			g_free(escaped);


			ArtSearch *search = NULL;
			LomoStream *stream = adb_get_stream_from_uri(adb, (gchar*) sqlite3_column_text(stmt, 3));
			if (stream != NULL)
				search = art_search(art, stream, (ArtFunc) queryer_search_cb, self);

			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(self->queryer_results), &iter);
			gtk_list_store_set(GTK_LIST_STORE(self->queryer_results), &iter,
				QUERYER_COLUMN_SEARCH, search,
				QUERYER_COLUMN_MATCH_TYPE, match_t[i],
				QUERYER_COLUMN_FULL_MATCH, fullmatch,
				QUERYER_COLUMN_TEXT, markup,
				-1);

			g_free(markup);
		}
		sqlite3_finalize(stmt);
	}
}

static void
queryer_load_query(Recently *self, gint matchtype, gchar *fullmatch)
{
	const gchar *keys[MATCH_N_TYPES] = { "uri", "artist", "album", "title" };

	g_return_if_fail((matchtype >= 0) && (matchtype < MATCH_N_TYPES) && (keys[matchtype] != NULL));

	GList *uris = NULL;
	char *q = sqlite3_mprintf(
		"SELECT uri FROM streams WHERE sid IN"
		"	(SELECT sid FROM metadata WHERE key='%q' AND value='%q');",
		keys[matchtype], fullmatch);

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
	LomoPlayer *lomo = GEL_APP_GET_LOMO(self->app);
	g_return_if_fail(lomo != NULL);

	GList *s = gtk_icon_view_get_selected_items(search_view);
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_icon_view_get_model(search_view));
	GtkTreeIter iter;
	GList *i = s;

	if (s)
		lomo_player_clear(lomo);
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
	if (s)
	{
		lomo_player_play(lomo, NULL);
		eina_plugin_switch_dock_widget(self->plugin, "playlist");
	}
	gel_list_deep_free(s, (GFunc) gtk_tree_path_free);
}

static void
queryer_search_cb(Art *art, ArtSearch *search, Recently *self)
{
	GtkTreeIter iter;
	if (list_store_get_iter_for_search(self->queryer_results, QUERYER_COLUMN_SEARCH, search, &iter))
		list_store_set_cover(self->queryer_results, QUERYER_COLUMN_COVER, &iter, art_search_get_result(search), QUERYER_COVER_SIZE);
	g_object_unref(G_OBJECT(art_search_get_stream(search)));

}

static gboolean
queryer_tree_model_filter_visible_cb(GtkTreeModel *model, GtkTreeIter *iter, Recently *self)
{
	gchar *str1 = NULL;
	gtk_tree_model_get(model, iter, QUERYER_COLUMN_FULL_MATCH, &str1, -1);
	gchar *str2 = (gchar *) gtk_entry_get_text(self->search_entry);

	gchar *str1_lc = g_utf8_strdown(str1, -1);
	gchar *str2_lc = g_utf8_strdown(str2, -1);
	g_free(str1);

	gchar *str1_normal = g_utf8_normalize (str1_lc, -1, G_NORMALIZE_NFKD);
	gchar *str2_normal = g_utf8_normalize (str2_lc, -1, G_NORMALIZE_NFKD);
	g_free(str1_lc);
	g_free(str2_lc);

	gboolean ret = (str1_normal && str2_normal && strstr(str1_normal, str2_normal));

	g_free(str1_normal);
	g_free(str2_normal);

	return ret;
}

// --
// Callbacks
// --
static void
lomo_clear_cb(LomoPlayer *lomo, Recently *self)
{
	Adb *adb = GEL_APP_GET_ADB(self->app);
	g_return_if_fail(adb != NULL);

	gchar **timestamps = adb_get_n_timestamps(adb, 1);
	g_return_if_fail(timestamps && timestamps[0]);

	gchar *ts = timestamps[0];
	gchar *title = adb_get_title_for_timestamp(adb, ts);
	gchar *escaped = g_markup_escape_text(title, -1);
	g_free(title);
	gchar *markup = g_strdup_printf("<b>%s</b>\n\t%s", stamp_to_human(ts), escaped);
	g_free(escaped);

	ArtSearch *search = NULL;
	LomoStream *stream = adb_get_stream_from_timestamp(adb, ts);
	if (stream)
		search = art_search(GEL_APP_GET_ART(self->app), stream, (ArtFunc) recently_search_cb, self);

	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first((GtkTreeModel*) self->pls_model, &iter))
	{
		gel_error(N_("Cannot get first iter"));
		g_free(markup);
		g_strfreev(timestamps);
		return;
	}
	gtk_list_store_prepend((GtkListStore *) self->pls_model, &iter);
	gtk_list_store_set((GtkListStore *) self->pls_model, &iter,
			RECENTLY_COLUMN_TIMESTAMP, ts,
			RECENTLY_COLUMN_SEARCH, search,
			RECENTLY_COLUMN_COVER, NULL,
			RECENTLY_COLUMN_MARKUP, markup,
			RECENTLY_COLUMN_PLAY, GTK_STOCK_MEDIA_PLAY,
			RECENTLY_COLUMN_ENQUEUE, EINA_STOCK_QUEUE,
			RECENTLY_COLUMN_DELETE, GTK_STOCK_DELETE,
			-1);
	g_free(markup);
	g_strfreev(timestamps);
}

// --
// Eina Plugin interface
// --
gboolean
recently_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	// Upgrade database
	Adb *adb;
	if ((adb = GEL_APP_GET_ADB(app)) == NULL)
	{
		g_set_error(error, recently_quark(), RECENTLY_ERROR_CANNOT_FETCH_ADB, N_("Cannot fetch Adb object"));
		return FALSE;
	}

	gpointer upgrades[] = {
		adb_upgrade_0, NULL
	};
	if (!adb_schema_upgrade(adb, "recently", upgrades, NULL, error))
		return FALSE;

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
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	if (lomo)
		g_signal_handlers_disconnect_by_func(lomo, "clear", lomo_clear_cb);
	eina_plugin_remove_dock_widget(plugin, "recently");
	g_free(plugin->data);
	return TRUE;
}

EINA_PLUGIN_SPEC(recently,
	PACKAGE_VERSION, "adb",
	NULL, NULL,

	N_("Stores your recent playlists"),
	N_("Recently plugin. ADB based version"),
	"adb.png",

	recently_plugin_init, recently_plugin_fini
	);

