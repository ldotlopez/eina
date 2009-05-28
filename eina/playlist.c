/*
 * eina/playlist.c
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

#define GEL_DOMAIN "Eina::Playlist"

#include <config.h>

// Std
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// GTK
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

// Lomo
#include <lomo/lomo-util.h>

// Gel
#include <gel/gel-io.h>
#include <gel/gel-ui.h>

// Modules
#include <eina/eina-plugin.h>
#include <eina/playlist.h>
#include <eina/dock.h>

// Widgets and utils
#include <eina/eina-file-chooser-dialog.h>
#include <eina/fs.h>

struct _EinaPlaylist
{
	EinaObj       parent;

	EinaConf     *conf;

	GtkWidget    *dock;
	GtkTreeView  *tv;
	gchar  *stream_fmt;
};

typedef enum
{
	PLAYLIST_COLUMN_STREAM = 0,
	PLAYLIST_COLUMN_STATE,
	PLAYLIST_COLUMN_TEXT,
	PLAYLIST_COLUMN_MARKUP,

	PLAYLIST_N_COLUMNS
} EinaPlaylistColumn;

// API
gboolean
eina_playlist_dock_init(EinaPlaylist *self);
static void
remove_selected(EinaPlaylist *self);
static void
setup_dnd(EinaPlaylist *self);
static gchar *
format_stream(LomoStream *stream, gchar *fmt);
static gchar*
format_stream_cb(gchar key, LomoStream *stream);

void
eina_playlist_update_item(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...);
#define eina_playlist_update_current_item(self,...) \
	eina_playlist_update_item(self, NULL,lomo_player_get_current(eina_obj_get_lomo(self)),__VA_ARGS__)

gboolean
__eina_playlist_search_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, EinaPlaylist *self);

// UI Callbacks
void
dock_row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self);
static void
add_button_clicked_cb
(GtkWidget *w, EinaPlaylist *self);
static gint 
sort_int_desc(gconstpointer a, gconstpointer b);
void
on_pl_remove_button_clicked(GtkWidget *w, EinaPlaylist *self);
void // for swapped
on_pl_clear_button_clicked (GtkWidget *w, EinaPlaylist *self);
void
on_pl_random_button_toggled(GtkWidget *w, EinaPlaylist *self);
void
on_pl_repeat_button_toggled(GtkWidget *w, EinaPlaylist *self);

// Lomo callbacks
static void lomo_state_change_cb
(LomoPlayer *lomo, EinaPlaylist *self);
static void lomo_insert_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
static void lomo_remove_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
static void lomo_change_cb
(LomoPlayer *lomo, gint old, gint new, EinaPlaylist *self);
static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlaylist *self);
static void lomo_random_cb
(LomoPlayer *lomo, gboolean val, EinaPlaylist *self);
static void lomo_eos_cb
(LomoPlayer *lomo, EinaPlaylist *self);
static void lomo_error_cb
(LomoPlayer *lomo, LomoStream *stream, GError *error, EinaPlaylist *self);
static void lomo_all_tags_cb
(LomoPlayer *lomo, LomoStream *stream, EinaPlaylist *self);

// UI callbacks
void dock_row_activated_cb
(GtkWidget *w,
 	GtkTreePath *path,
	GtkTreeViewColumn *column,
	EinaPlaylist *self);
void add_button_clicked_cb
(GtkWidget *w, EinaPlaylist *self);
void on_pl_clear_button_clicked
(GtkWidget *w, EinaPlaylist *self);
void on_pl_remove_button_clicked
(GtkWidget *w, EinaPlaylist *self);
void on_pl_random_button_toggled
(GtkWidget *w, EinaPlaylist *self);
void on_pl_repeat_button_toggled
(GtkWidget *w, EinaPlaylist *self);
void on_pl_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist     *self);
void on_pl_drag_data_get
(GtkWidget *w,
	GdkDragContext   *drag_context,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist     *self);

void on_pl_settings_change
(EinaConf *conf, const gchar *key, gpointer data);

/* Signal definitions */
GelUISignalDef _playlist_signals[] = {
	{ "playlist-treeview",   "row-activated",
		G_CALLBACK(dock_row_activated_cb) },

	{ "playlist-repeat-button", "toggled",
	G_CALLBACK(on_pl_repeat_button_toggled) },
	{ "playlist-random-button", "toggled",
	G_CALLBACK(on_pl_random_button_toggled) },
	
	{ "playlist-add-button",  "clicked",
	G_CALLBACK(add_button_clicked_cb) },
	{ "playlist-clear-button",  "clicked",
	G_CALLBACK(on_pl_clear_button_clicked) },
	{ "playlist-remove-button", "clicked",
	G_CALLBACK(on_pl_remove_button_clicked) },
	
	{ "playlist-treeview",   "drag-data-received",
	G_CALLBACK(on_pl_drag_data_received) },
	{ "playlist-treeview",   "drag-data-get",
	G_CALLBACK(on_pl_drag_data_get) },
	
	GEL_UI_SIGNAL_DEF_NONE
};

static gboolean
playlist_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *self;
	gboolean random, repeat;

	self = g_new0(EinaPlaylist, 1);
	if (!eina_obj_init(EINA_OBJ(self), app, "playlist", EINA_OBJ_GTK_UI, error))
	{
		g_free(self);
		return FALSE;
	}

	// Load settings module
	if ((self->conf = GEL_APP_GET_SETTINGS(app)) == NULL)
	{
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}
	g_signal_connect(self->conf, "change", G_CALLBACK(on_pl_settings_change), self);

	// Setup internal values from settings
	repeat = eina_conf_get_bool(self->conf, "/core/repeat", FALSE);
	random = eina_conf_get_bool(self->conf, "/core/random", FALSE);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(eina_obj_get_widget(self, "playlist-repeat-button")), repeat);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(eina_obj_get_widget(self, "playlist-random-button")), random);
	lomo_player_set_repeat(eina_obj_get_lomo(self), repeat);
	lomo_player_set_random(eina_obj_get_lomo(self), random);

	self->stream_fmt = (gchar *) eina_conf_get_str(self->conf, "/ui/playlist/fmt", "{%a - }%t");

	/* Setup stock icons */
	gtk_image_set_from_stock(eina_obj_get_typed(self, GTK_IMAGE, "playlist-random-image"), EINA_STOCK_RANDOM, GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock(eina_obj_get_typed(self, GTK_IMAGE, "playlist-repeat-image"), EINA_STOCK_REPEAT, GTK_ICON_SIZE_BUTTON);

	if (!eina_playlist_dock_init(self))
	{
		gel_error(N_("Cannot init dock widget"));
		return FALSE;
	}

	/* Connect some UI signals */
	gel_ui_signal_connect_from_def_multiple(
		eina_obj_get_ui(self),
		_playlist_signals,
		self,
		NULL);

	/* Connect lomo signals */
	g_signal_connect(eina_obj_get_lomo(self), "insert",   G_CALLBACK(lomo_insert_cb),      self);
	g_signal_connect(eina_obj_get_lomo(self), "remove",   G_CALLBACK(lomo_remove_cb),      self);
	g_signal_connect(eina_obj_get_lomo(self), "change",   G_CALLBACK(lomo_change_cb),   self);
	g_signal_connect(eina_obj_get_lomo(self), "clear",    G_CALLBACK(lomo_clear_cb),    self);
	g_signal_connect(eina_obj_get_lomo(self), "play",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "stop",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "pause",    G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "random",   G_CALLBACK(lomo_random_cb),   self);
	g_signal_connect(eina_obj_get_lomo(self), "eos",      G_CALLBACK(lomo_eos_cb),      self);
	g_signal_connect(eina_obj_get_lomo(self), "all-tags", G_CALLBACK(lomo_all_tags_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "error",    G_CALLBACK(lomo_error_cb),    self);

	/* Load playlist and go to last session's current */
	/*
	tmp = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "playlist", NULL);
	if (g_file_get_contents(tmp, &buff, NULL, NULL))
	{
		uris = g_uri_list_extract_uris((const gchar *) buff);
		lomo_player_append_uri_strv(eina_obj_get_lomo(self), uris);
		g_strfreev(uris);
	}
	g_free(tmp);
	g_free(buff);
	lomo_player_go_nth(
		eina_obj_get_lomo(self), 
		eina_conf_get_int(self->conf, "/playlist/last_current", 0),
		NULL);
	*/
	setup_dnd(self);

	gtk_widget_show(self->dock);
	return eina_dock_add_widget(GEL_APP_GET_DOCK(app), "playlist",
		gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU), self->dock);
}

static gboolean
playlist_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *self = GEL_APP_GET_PLAYLIST(app);
	gchar *file;
	gint i = 0;
	gint fd;

	GList *pl    = lomo_player_get_playlist(eina_obj_get_lomo(self));
	GList *iter  = pl;
	char **out = g_new0(gchar*, g_list_length(pl) + 1);
	while (iter)
	{
		out[i++] = lomo_stream_get_tag(LOMO_STREAM(iter->data), LOMO_TAG_URI);
		iter = iter->next;
	}
	g_list_free(pl);
	
	gchar *buff = g_strjoinv("\n", out);
	g_free(out);

	GError *err = NULL;
	file = gel_app_build_config_filename("playlist", TRUE, 0755, &err);
	if (file == NULL)
	{
		gel_warn("Cannot create playlist file: %s", err->message);
		g_error_free(err);
	}
	else
	{
		fd = g_open(file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR);
		if (write(fd, buff, strlen(buff)) == -1)
			; // XXX Just avoid warnings
		close(fd);
		g_free(file);
	}

	i = lomo_player_get_current(eina_obj_get_lomo(self));
	eina_conf_set_int(self->conf, "/playlist/last_current", i);
	eina_dock_remove_widget(EINA_OBJ_GET_DOCK(self), "playlist");

	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

static gboolean
dock_key_press_event_cb(GtkWidget *widget, GdkEvent  *event, EinaPlaylist *self) 
{
	if (event->type != GDK_KEY_PRESS)
		return FALSE;

	switch (event->key.keyval)
	{
	case GDK_Delete:
		remove_selected(self);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

gboolean
eina_playlist_dock_init(EinaPlaylist *self)
{
	self->tv = GTK_TREE_VIEW(eina_obj_get_widget(self, "playlist-treeview"));

	g_object_set(eina_obj_get_object(self, "title-renderer"),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_NONE,
		NULL);
	gtk_tree_view_set_search_column(self->tv, PLAYLIST_COLUMN_MARKUP);
	gtk_tree_view_set_search_equal_func(self->tv, (GtkTreeViewSearchEqualFunc) __eina_playlist_search_func, self, NULL);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->tv), GTK_SELECTION_MULTIPLE);

	self->dock = eina_obj_get_widget(self, "playlist-main-box");
	g_object_ref(self->dock);
	if (self->dock->parent != NULL)
		gtk_widget_unparent(self->dock);

	g_signal_connect(self->dock, "key-press-event", G_CALLBACK(dock_key_press_event_cb), self);
	return TRUE;
}

static void
remove_selected(EinaPlaylist *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model = NULL;
	gchar *path_str;
	GList *rows, *l, *sorted, *to_sort = NULL;

	// Get selected
	selection = gtk_tree_view_get_selection(self->tv);
	l = rows = gtk_tree_selection_get_selected_rows(selection, &model);

	// Create an integer list
	while (l)
	{
		path_str = gtk_tree_path_to_string((GtkTreePath*  )(l->data));
		gint index = (gint) (atol(path_str) / 1);

		to_sort = g_list_prepend(to_sort, GINT_TO_POINTER(index));

		g_free(path_str);
		l = l->next;
	}

	// Free results
	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	// Sort list and delete
	l = sorted = g_list_sort(to_sort, sort_int_desc);
	while (l) {
		lomo_player_del(eina_obj_get_lomo(self), GPOINTER_TO_INT(l->data));
		l = l->next;
	}
	g_list_free(sorted);
}

void
__eina_playlist_update_item_state(EinaPlaylist *self, GtkTreeIter *iter, gint item, gchar *current_state, gint current_item)
{
	LomoState state = LOMO_STATE_INVALID;
	gchar *new_state = NULL;

	// If item is not the active one, new_state is NULL
	if (item == current_item)
	{
		state = lomo_player_get_state(eina_obj_get_lomo(self));
		switch (state)
		{
			case LOMO_STATE_INVALID:
				// leave stock-id as NULL
				break;
			case LOMO_STATE_PLAY:
				 if (gtk_widget_get_direction((GtkWidget *)self->dock) == GTK_TEXT_DIR_LTR)
					new_state = "gtk-media-play-ltr";
				else
					new_state = "gtk-media-play-rtl";
				break;
			case LOMO_STATE_PAUSE:
				new_state = GTK_STOCK_MEDIA_PAUSE;
				break;
			case LOMO_STATE_STOP:
				new_state = GTK_STOCK_MEDIA_STOP;
				break;
			default:
				gel_warn("Unknow state: %d", state);
				break;
		}
	}

	// Update only if needed
	if ((current_state != NULL) && (new_state != NULL))
		if (g_str_equal(new_state, current_state))
			return;

	gel_debug("Updating item %d(cur %d): %s => %s", item, current_item, current_state, new_state);
	gtk_list_store_set((GtkListStore *) gtk_tree_view_get_model(self->tv), iter,
		PLAYLIST_COLUMN_STATE, new_state,
		-1);
}

void
__eina_playlist_update_item_title(EinaPlaylist *self, GtkTreeIter *iter, gint item, gchar *current_title, gint current_item, gchar *current_raw_title)
{
	gchar *markup = NULL;

	// Calculate new value
	if (item == current_item)
		markup = g_strdup_printf("<b>%s</b>", current_raw_title);
	else
		markup = g_strdup(current_raw_title);
	
	// Update only if needed
	if (!g_str_equal(current_title, markup))
	{
		gel_debug("Updating item %d(cur %d): %s => %s", item, current_item, current_title, markup);
		gtk_list_store_set((GtkListStore *) gtk_tree_view_get_model(self->tv), iter,
			PLAYLIST_COLUMN_MARKUP, markup,
			-1);
	}
	g_free(markup);
}

void
eina_playlist_update_item(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...)
{
	GtkTreePath *treepath;
	GtkTreeIter  _iter;
	gint*        treepath_indices; // Don't free, it's owned by treepath
	gint   current = -1;

	gchar *state     = NULL;
	gchar *title     = NULL;
	gchar *raw_title = NULL;

	va_list columns;
	gint column;

	if ((item == -1) && (iter == NULL))
	{
		gel_error("You must supply an iter _or_ an item");
		return;
	}

	// Get item from iter
	if (item == -1)
	{
		treepath = gtk_tree_model_get_path(gtk_tree_view_get_model(self->tv), iter);
		if (treepath == NULL)
		{
			gel_warn("Cannot get a GtkTreePath from index %d", item);
			return;
		}
		treepath_indices = gtk_tree_path_get_indices(treepath);
		item = treepath_indices[0];
		gtk_tree_path_free(treepath);
	}

	// Get iter from item
	if (iter == NULL)
	{
		treepath = gtk_tree_path_new_from_indices(item, -1);
		gtk_tree_model_get_iter(gtk_tree_view_get_model(self->tv), &_iter, treepath);
		gtk_tree_path_free(treepath);
		iter = &_iter;
	}

	current = lomo_player_get_current(eina_obj_get_lomo(self));

	// Get data from GtkListStore supported columns
	gtk_tree_model_get(gtk_tree_view_get_model(self->tv), iter,
		PLAYLIST_COLUMN_STATE, &state,
		PLAYLIST_COLUMN_MARKUP, &title,
		PLAYLIST_COLUMN_TEXT, &raw_title,
		-1);

	// Start updating columns
	va_start(columns, item);
	do {
		column = va_arg(columns,gint);
		if (column == -1)
			break;

		switch (column)
		{
			case PLAYLIST_COLUMN_STATE:
				__eina_playlist_update_item_state(self, iter, item, state, current);
				break;

			case PLAYLIST_COLUMN_MARKUP:
				__eina_playlist_update_item_title(self, iter, item, title, current, raw_title);
				break;

			default:
				gel_warn("Unknow column %d", column);
				break;
		}
	} while(column != -1);

	g_free(state);
	g_free(title);
	g_free(raw_title);
}

gboolean
__eina_playlist_search_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, EinaPlaylist *self)
{
	gchar *title = NULL, *title_lc, *key_lc;
	gboolean ret = FALSE;

	if (model == NULL)
		model = gtk_tree_view_get_model(self->tv);

	if (column != PLAYLIST_COLUMN_MARKUP)
	{
		gel_warn("Invalid search column %d", column);
		return TRUE;
	}
	gtk_tree_model_get(model, iter,
		PLAYLIST_COLUMN_MARKUP, &title,
		-1);
	title_lc = g_utf8_strdown(title, -1);
	key_lc = g_utf8_strdown(key, -1);
	g_free(title);

	if (strstr(title_lc, key_lc) != NULL)
		ret = FALSE;
	else
		ret = TRUE;
	g_free(title_lc);
	g_free(key_lc);

	return ret;
}

/*
 * UI Callbacks
 */
void
dock_row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self)
{
	gint *indexes;
	GError *err = NULL;

	indexes = gtk_tree_path_get_indices(path);
	lomo_player_go_nth(eina_obj_get_lomo(self), indexes[0], NULL);
	if (lomo_player_get_state(eina_obj_get_lomo(self)) != LOMO_STATE_PLAY )
	{
		lomo_player_play(eina_obj_get_lomo(self), &err);
		if (err)
		{
			gel_error("Cannot set state to LOMO_STATE_PLAY: %s", err->message);
			g_error_free(err);
		}
	}
}

static void
add_button_clicked_cb
(GtkWidget *w, EinaPlaylist *self)
{
	eina_fs_file_chooser_load_files(eina_obj_get_lomo(self));
}

static gint
sort_int_desc(gconstpointer a, gconstpointer b)
{
	gint int_a = GPOINTER_TO_INT(a);
	gint int_b = GPOINTER_TO_INT(b);
	if (int_a > int_b) return -1;
	if (int_a < int_b) return  1;
	return 0;
}

static gchar *
format_stream(LomoStream *stream, gchar *fmt)
{
	if (lomo_stream_get_all_tags_flag(stream))
		return gel_str_parser(fmt, (GelStrParserFunc) format_stream_cb, stream);
	else
	{
		gchar *unescape_uri = g_uri_unescape_string(lomo_stream_get_tag(stream, LOMO_TAG_URI), NULL);
		gchar *ret = g_path_get_basename(unescape_uri);
		g_free(unescape_uri);
		return ret;
	}
}

static gchar*
format_stream_cb(gchar key, LomoStream *stream)
{
	gchar *tag = lomo_stream_get_tag_by_id(stream, key);
	if ((tag == NULL) && (key == 't'))
	{
		tag = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
	}

	return tag;
}

void
on_pl_remove_button_clicked(GtkWidget *w, EinaPlaylist *self)
{
	remove_selected(self);
}

void
on_pl_clear_button_clicked (GtkWidget *w, EinaPlaylist *self)
{
	lomo_player_clear(eina_obj_get_lomo(self));
}

void
on_pl_random_button_toggled(GtkWidget *w, EinaPlaylist *self)
{
	lomo_player_set_random(
		eina_obj_get_lomo(self),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
	eina_conf_set_bool(
		self->conf,
		"/core/random",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

void
on_pl_repeat_button_toggled(GtkWidget *w, EinaPlaylist *self)
{
	lomo_player_set_repeat(
		eina_obj_get_lomo(self),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
	eina_conf_set_bool(
		self->conf,
		"/core/repeat",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

// --
// Lomo callbacks
// --
static void lomo_state_change_cb
(LomoPlayer *lomo, EinaPlaylist *self)
{
	eina_playlist_update_current_item(self, PLAYLIST_COLUMN_STATE, -1);
}

static void lomo_insert_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	gchar *text = format_stream(stream, self->stream_fmt);
	gchar *v = g_markup_escape_text(text, -1);
	g_free(text);

	gchar *m = NULL;
	if (pos == lomo_player_get_current(lomo))
		m = g_strdup_printf("<b>%s</b>", v);

	GtkTreeIter iter;
	gtk_list_store_insert_with_values((GtkListStore *) gtk_tree_view_get_model(self->tv),
		&iter, pos,
		PLAYLIST_COLUMN_STREAM, stream,
		PLAYLIST_COLUMN_STATE,  NULL,
		PLAYLIST_COLUMN_TEXT,   v,
		PLAYLIST_COLUMN_MARKUP, m ? m : v,
		-1);
	g_free(v);
	gel_free_and_invalidate(m, NULL, g_free);
}

static void lomo_remove_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *path_str;
	gchar        *raw_title;
	path_str = g_strdup_printf("%d", pos);

	model = gtk_tree_view_get_model(self->tv);
	if (!gtk_tree_model_get_iter_from_string(model, &iter, (const gchar *) path_str))
	{
		gel_error("Cannot find iter corresponding to index %d", pos);
		g_free(path_str);
		return;
	}

	gtk_tree_model_get(model, &iter,
		PLAYLIST_COLUMN_TEXT, &raw_title,
		-1);
	
	if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
	{
		gel_error("Cannot remove iter");
		return;
	}

	g_free(path_str);
}

static void lomo_change_cb
(LomoPlayer *lomo, gint old, gint new, EinaPlaylist *self)
{
	g_return_if_fail((old >= 0) || (new >= 0));

	// Restore normal (null) state on old stream except it was failed
	if (old >= 0)
	{
		if (lomo_stream_get_failed_flag(lomo_player_nth_stream(lomo, old)))
			eina_playlist_update_item(self, NULL, old, PLAYLIST_COLUMN_MARKUP, -1);
		else
			eina_playlist_update_item(self, NULL, old, PLAYLIST_COLUMN_STATE, PLAYLIST_COLUMN_MARKUP, -1);
	}

	// Create markup for the new stream
	if (new >= 0)
	{
		eina_playlist_update_item(self, NULL, new, PLAYLIST_COLUMN_STATE, PLAYLIST_COLUMN_MARKUP, -1);

		GtkTreePath *treepath = gtk_tree_path_new_from_indices(new, -1);
		gtk_tree_view_scroll_to_cell(self->tv, treepath, NULL,
			TRUE, 0.5, 0.0);
		gtk_tree_path_free(treepath);
	}
}

static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlaylist *self)
{
	gtk_list_store_clear((GtkListStore *) gtk_tree_view_get_model(self->tv));
}

static void lomo_random_cb
(LomoPlayer *lomo, gboolean val, EinaPlaylist *self)
{
	if (val)
		lomo_player_randomize(lomo);
}

static gboolean
lomo_eos_cb_helper(LomoPlayer *lomo)
{
	if (lomo_player_get_next(lomo) != -1) {
		lomo_player_go_next(lomo, NULL);
		lomo_player_play(lomo, NULL); //XXX: Handle GError
	}

	return FALSE;
}

static void lomo_eos_cb
(LomoPlayer *lomo, EinaPlaylist *self)
{
	g_idle_add((GSourceFunc) lomo_eos_cb_helper, lomo);
}

static void lomo_error_cb
(LomoPlayer *lomo, LomoStream *stream, GError *error, EinaPlaylist *self)
{
	gel_error("Got error on stream %p: %s", stream, error->message);

	if ((stream == NULL) || !lomo_stream_get_failed_flag(stream))
		return;

	gint pos;
	if ((pos = lomo_player_index(lomo, stream)) == -1)
	{
		gel_warn("Stream %p doest belongs to Player %p", stream, lomo);
		return;
	}

	GtkTreeIter iter;
	GtkTreePath *treepath = gtk_tree_path_new_from_indices(pos, -1);

	if (!gtk_tree_model_get_iter(gtk_tree_view_get_model(self->tv), &iter, treepath))
	{
		gel_warn("Cannot get iter for stream %p", stream);
		gtk_tree_path_free(treepath);
		return;
	}
	gtk_tree_path_free(treepath);
		
	gtk_list_store_set((GtkListStore *) gtk_tree_view_get_model(self->tv), &iter,
		PLAYLIST_COLUMN_STATE, GTK_STOCK_STOP,
		-1);

	lomo_player_go_next(lomo, NULL);
	lomo_player_play(lomo, NULL);
}

static void lomo_all_tags_cb
(LomoPlayer *lomo, LomoStream *stream, EinaPlaylist *self)
{
	GtkTreeModel *model = gtk_tree_view_get_model(self->tv);
	gint index = lomo_player_index(lomo, stream);
	if (index < 0)
	{
		gel_error(N_("Cannot find index for stream %p (%s)"), stream, lomo_stream_get_tag(stream, LOMO_TAG_URI));
		return;
	}

	GtkTreePath *treepath = gtk_tree_path_new_from_indices(index, -1);
	GtkTreeIter iter;
	if (!treepath || !gtk_tree_model_get_iter(model, &iter, treepath))
	{
		gel_error(N_("Cannot get iter for index %d"), index);
		return;
	}
	gtk_tree_path_free(treepath);

	gchar *text = format_stream(stream, self->stream_fmt);
	gchar *v = g_markup_escape_text(text, -1);
	g_free(text);

	gchar *m = NULL;
	if (index == lomo_player_get_current(lomo))
		m = g_strdup_printf("<b>%s</b>", v);

	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		PLAYLIST_COLUMN_TEXT, v,
		PLAYLIST_COLUMN_MARKUP, m ? m : v,
		-1);
	g_free(v);
	gel_free_and_invalidate(m, NULL, g_free);
}


void on_pl_drag_data_received(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist       *self) {
#if 0
	gchar **uris;
	gint i;
	gchar *filename;
	GSList *uri_list = NULL, *uri_scan, *uri_filter;

	/* Get the uri list from DnD and convert it to a
	 * file:///path/to/filename GSList
	 *
	 * This list (uri_list) has strings on a newly allocated memory
	 */
	uris = g_uri_list_extract_uris((gchar *) data->data);
	for ( i = 0; uris[i] != NULL; i++ ) {
		filename = g_filename_from_uri(uris[i], NULL, NULL);
		uri_list = g_slist_prepend(uri_list, g_strconcat("file://", filename, NULL));
		g_free(filename);
	}
	uri_list = g_slist_reverse(uri_list);
	g_strfreev(uris);

	/*
	 * Scan files over uri_list, then free uri_list
	 * since eina_fs_scan duplicates data
	 */
	uri_scan = eina_fs_scan(uri_list);
	g_ext_slist_free(uri_list, g_free);

	/*
	 * Apply filters on the uri_scan GSList.
	 * Warn: We dont use eina_fs_filter_filter's duplicate flag
	 * so, we dont replicate data
	 */
	uri_filter = eina_fs_filter_filter(self->filter_suffixes, // Filter
		uri_scan,
		TRUE, FALSE, FALSE,
		FALSE, FALSE);
	
	/* Add valid uris to LomoPlayer */
	lomo_player_append_uri_multi(eina_obj_get_lomo(self), (GList *) uri_filter);

	/* Free our dirty data */
	g_ext_slist_free(uri_scan, g_free);
	g_slist_free(uri_filter);
#endif
}

void on_pl_drag_data_get(GtkWidget *w,
	GdkDragContext   *drag_context,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist     *self) {
#if 0
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreePath      *treepath;
	GtkTreeIter       iter;
	GList            *rows, *l;
	gint              pos;
	gchar           **charv;
	gint              i;
	LomoStream       *stream;
	gchar *tmp = NULL;

	/* Process:
	 * 1. Get the list of rows selected
	 * 2. Get GtkTreeIter for each GtkTreePath
	 * 3. Extract ID
	 * 4. Query LomoPlayer to get full URI and save it
	 */
	selection = gtk_tree_view_get_selection(self->tv);
	model = (GtkTreeModel *) (self->model);
	l = rows = gtk_tree_selection_get_selected_rows(selection, &model);

	charv = (gchar **) g_new0(gchar*, g_list_length(l));
	
	i = 0;
	while (l) {
		treepath = (GtkTreePath *) l->data;
		if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(self->model), &iter, treepath)) {
			continue;
		}

		gtk_tree_model_get(GTK_TREE_MODEL(self->model), &iter, PLAYLIST_COLUMN_NUMBER, &pos, -1);
		stream = (LomoStream *) lomo_player_nth_stream(eina_obj_get_lomo(self), pos);
		charv[i] = lomo_stream_get_tag(stream, LOMO_TAG_URI); // Dont copy, its rigth.

		i++;
		l = l->next;
	}
	
	/* End Process */
	tmp = g_strjoinv("\r\n", charv);
	gtk_selection_data_set (data, data->target, 8, (guchar*) tmp, strlen(tmp));
	g_free(charv); // No g_strfreev, its rigth

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows); // g_list_free dont revised

	gtk_drag_finish(drag_context, TRUE, FALSE, time);
#endif
}

void on_pl_settings_change(EinaConf *conf, const gchar *key, gpointer data) {
	EinaPlaylist *self = (EinaPlaylist *) data;
	if (g_str_equal(key, "/core/repeat")) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(eina_obj_get_widget(self, "playlist-repeat-button")),
			eina_conf_get_bool(conf, "/core/repeat", TRUE));
	}

	else if (g_str_equal(key, "/core/random")) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(eina_obj_get_widget(self, "playlist-random-button")),
			eina_conf_get_bool(conf, "/core/random", TRUE));
	}
}

// --
// DnD code
// --
enum {
	TARGET_INT32,
	TARGET_STRING,
	TARGET_ROOTWIN
};

/* datatype (string), restrictions on DnD
 * (GtkTargetFlags), datatype (int) */
static GtkTargetEntry target_list[] = {
	// { "INTEGER",    0, TARGET_INT32 },
	{ "STRING",     0, TARGET_STRING },
	{ "text/plain", 0, TARGET_STRING },
	// { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = G_N_ELEMENTS (target_list);

static void
list_read_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data)
{
	EinaPlaylist *self = EINA_PLAYLIST(data);
	GList  *uris;
	GSList *filter;

	if (!(uris = gel_io_op_result_get_object_list(res)))
	{
		gel_io_op_unref(op);
		return;
	}
	filter = gel_list_filter(uris, (GelFilterFunc) eina_fs_is_supported_file, NULL);
	if (!filter)
	{
		gel_io_op_unref(op);
		return;
	}

	GList  *lomofeed = NULL;
	GSList *iter = filter;
	while (iter)
	{
		lomofeed = g_list_prepend(lomofeed, g_file_get_uri(G_FILE(iter->data)));
		iter = iter->next;
	}
	lomofeed = g_list_reverse(lomofeed);

	g_slist_free(filter);
	gel_io_op_unref(op);

	lomo_player_append_uri_multi(eina_obj_get_lomo(self), lomofeed);
	gel_list_deep_free(lomofeed, g_free);
}

static void
list_read_error_cb(GelIOOp *op, GFile *source, GError *err, gpointer data)
{
	gel_warn("Error");
}

static void
drag_data_received_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	GtkSelectionData *selection_data, guint target_type, guint time,
	gpointer data)
{
	EinaPlaylist *self = EINA_PLAYLIST(data);
	gchar *string = NULL;

	// Check for data
	if ((selection_data == NULL) || (selection_data->length < 0))
	{
		gel_warn("Got invalid selection from DnD");
		return gtk_drag_finish (context, FALSE, FALSE, time);
	}

	if (context->action != GDK_ACTION_COPY)
	{
		gel_warn("Got invalid action from DnD");
		return gtk_drag_finish (context, FALSE, FALSE, time);
	}

	/*
	if (context-> action == GDK_ACTION_ASK)
	{
		// Ask the user to move or copy, then set the context action.
	}

	if (context-> action == GDK_ACTION_MOVE)
		delete_selection_data = TRUE;
	*/

	/* Check that we got the format we can use */
	switch (target_type)
	{
	case TARGET_STRING:
		string = (gchar*) selection_data-> data;
		break;
	default:
		gel_warn("Unknow target type in DnD");
	}

	if (string == NULL)
		return gtk_drag_finish (context, FALSE, FALSE, time);


	// Parse
	gint i;
	gchar **uris = g_uri_list_extract_uris(string);
	gtk_drag_finish (context, TRUE, FALSE, time);
	GSList *files = NULL;
	for (i = 0; uris[i] && uris[i][0]; i++)
		files = g_slist_prepend(files, g_file_new_for_uri(uris[i]));

	g_strfreev(uris);
	files = g_slist_reverse(files);

	gel_io_list_read(files, "standard::*",
		list_read_success_cb, list_read_error_cb,
		self);
}

void setup_dnd(EinaPlaylist *self)
{
	GtkWidget *well_dest = eina_obj_get_widget(self, "playlist-treeview");
	gtk_drag_dest_set(well_dest,
		GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_MOTION, // motion or highlight can do c00l things
		target_list,            /* lists of target to support */
		n_targets,              /* size of list */
		GDK_ACTION_COPY);
	g_signal_connect (well_dest, "drag-data-received",
		G_CALLBACK(drag_data_received_cb), self);
/*
	g_signal_connect (well_dest, "drag-leave",
		G_CALLBACK (drag_leave_handl), NULL);
	g_signal_connect (well_dest, "drag-motion",
		G_CALLBACK (drag_motion_handl), NULL);
	g_signal_connect (well_dest, "drag-drop",
		G_CALLBACK (drag_drop_handl), NULL);
*/
}

/*
 * Connector
 */
G_MODULE_EXPORT GelPlugin playlist_plugin = {
	GEL_PLUGIN_SERIAL,
	"playlist", PACKAGE_VERSION, "dock",
	NULL, NULL,

	N_("Build-in playlist plugin"), NULL, NULL,

	playlist_init, playlist_fini,
	NULL, NULL, NULL
};

