/*
 * eina/playlist.c
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

// Gel
#include <gel/gel-io.h>
#include <gel/gel-ui.h>

// Modules
#include <eina/eina-plugin.h>
#include <eina/playlist.h>

// Widgets and utils
#include <eina/fs.h>
#include <eina/window.h>

#define TAB_PLAYLIST_EMPTY 0
#define TAB_PLAYLIST_NON_EMPTY 1

struct _EinaPlaylist
{
	EinaObj       parent;

	GtkWidget    *dock;
	GtkTreeView  *tv;
	GtkTreeModel *model; // Model, tv has a filter attached

	gchar        *stream_fmt;
	GQueue       *io_tree_files;
	GelIOTreeOp  *io_op;
};

typedef enum
{
	PLAYLIST_COLUMN_STREAM = 0,
	PLAYLIST_COLUMN_STATE,
	PLAYLIST_COLUMN_TEXT,
	PLAYLIST_COLUMN_MARKUP,
	PLAYLIST_COLUMN_INDEX,
	PLAYLIST_COLUMN_QUEUE_STR,

	PLAYLIST_N_COLUMNS
} EinaPlaylistColumn;

// API
static void
playlist_set_valist_from_stream(EinaPlaylist *self, LomoStream *stream, ...);
gboolean
eina_playlist_dock_init(EinaPlaylist *self);
static gboolean
playlist_resize_columns(EinaPlaylist *self);
static void
playlist_queue_selected(EinaPlaylist *self);
static void
playlist_remove_selected(EinaPlaylist *self);
static void
playlist_update_tab(EinaPlaylist *self);
static void
playlist_dnd_setup(EinaPlaylist *self);
static gchar *
format_stream(LomoStream *stream, gchar *fmt);
static gchar*
format_stream_cb(gchar key, LomoStream *stream);

void
eina_playlist_update_item(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...);
#define eina_playlist_update_current_item(self,...) \
	eina_playlist_update_item(self, NULL,lomo_player_get_current(eina_obj_get_lomo(self)),__VA_ARGS__)

// UI Callbacks
void
dock_row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self);
gboolean
treeview_button_press_event_cb(GtkWidget *w, GdkEventButton *ev, EinaPlaylist *self);
void
action_activate_cb(GtkAction *action, EinaPlaylist *self);

// Lomo callbacks
static void lomo_state_change_cb
(LomoPlayer *lomo, EinaPlaylist *self);
static void lomo_insert_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
static void lomo_remove_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
static void lomo_queue_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
static void lomo_dequeue_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
static void lomo_change_cb
(LomoPlayer *lomo, gint old, gint new, EinaPlaylist *self);
static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlaylist *self);
static void lomo_eos_cb
(LomoPlayer *lomo, EinaPlaylist *self);
static void lomo_error_cb
(LomoPlayer *lomo, LomoStream *stream, GError *error, EinaPlaylist *self);
static void lomo_all_tags_cb
(LomoPlayer *lomo, LomoStream *stream, EinaPlaylist *self);

// Handle focus events
static gboolean
widget_focus_in_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self);
static gboolean
widget_focus_out_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self);

// UI callbacks
static void
playlist_treeview_size_allocate_cb(GtkWidget *w, GtkAllocation *a, EinaPlaylist *self);
void dock_row_activated_cb
(GtkWidget *w,
 	GtkTreePath *path,
	GtkTreeViewColumn *column,
	EinaPlaylist *self);

static gboolean
widget_focus_in_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self);
static gboolean
widget_focus_out_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self);

static void search_entry_changed_cb
(GtkWidget *w, EinaPlaylist *self);
static void search_entry_icon_press_cb
(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, EinaPlaylist *self);

static void
io_tree_read_error_cb(GelIOTreeOp *op, const GFile *source, const GError *error, gpointer data);
static void
io_tree_read_success_cb(GelIOTreeOp *op, const GFile *source, const GNode *result, EinaPlaylist *self);

/* Signal definitions */
GelUISignalDef _playlist_signals[] = {
	{ "playlist-treeview",   "row-activated",
	G_CALLBACK(dock_row_activated_cb) },
	{ "playlist-treeview",   "size-allocate",
	G_CALLBACK(playlist_treeview_size_allocate_cb) },

	{ "playlist-search-entry", "changed",
	G_CALLBACK(search_entry_changed_cb) },
	{ "playlist-search-entry", "icon-press",
	G_CALLBACK(search_entry_icon_press_cb) },
	{ "playlist-search-entry", "activate",
	G_CALLBACK(gtk_widget_grab_focus) },
	{ "playlist-search-entry", "focus-in-event",
	G_CALLBACK(widget_focus_in_event_cb) },
	{ "playlist-search-entry", "focus-out-event",
	G_CALLBACK(widget_focus_out_event_cb) },

	{ "playlist-treeview", "button-press-event",
	G_CALLBACK(treeview_button_press_event_cb) },

	GEL_UI_SIGNAL_DEF_NONE
};

/*
// Keybindings
static EinaWindowKeyBind keybindings[] = {
	{ GDK_Control_L | GDK_f, (EinaWindowKeyBindHandler) window_keybind_handler },
	{ GDK_Control_R | GDK_f, (EinaWindowKeyBindHandler) window_keybind_handler } 
};
*/

G_MODULE_EXPORT gboolean
playlist_plugin_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *self;

	self = g_new0(EinaPlaylist, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "playlist", EINA_OBJ_GTK_UI, error))
	{
		g_free(self);
		return FALSE;
	}

	// Configure settings
	self->stream_fmt = (gchar *) eina_conf_get_str(eina_obj_get_settings((EinaObj *) self), "/ui/playlist/fmt", "{%a - }%t");

	if (!eina_playlist_dock_init(self))
	{
		gel_error(N_("Cannot init dock widget"));
		return FALSE;
	}
	self->io_tree_files = g_queue_new();

	// Connect some UI signals
	gel_ui_signal_connect_from_def_multiple(eina_obj_get_ui(self), _playlist_signals, self, NULL);

	// Actions
	gint i;
	const gchar *actions[] = { "add-action", "remove-action", "clear-action", "queue-action", "dequeue-action" };
	for (i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GObject *action = eina_obj_get_object(self, actions[i]);
		if (action == NULL)
		{
			gel_warn(N_("Missing action: %s"), actions[i]);
			continue;
		}
		g_signal_connect(action, "activate", (GCallback) action_activate_cb, self);
	}

	// Lomo signals
	g_signal_connect(eina_obj_get_lomo(self), "insert",   G_CALLBACK(lomo_insert_cb),       self);
	g_signal_connect(eina_obj_get_lomo(self), "remove",   G_CALLBACK(lomo_remove_cb),       self);
	g_signal_connect(eina_obj_get_lomo(self), "queue",    G_CALLBACK(lomo_queue_cb),        self);
	g_signal_connect(eina_obj_get_lomo(self), "dequeue",  G_CALLBACK(lomo_dequeue_cb),      self);
	g_signal_connect(eina_obj_get_lomo(self), "change",   G_CALLBACK(lomo_change_cb),       self);
	g_signal_connect(eina_obj_get_lomo(self), "clear",    G_CALLBACK(lomo_clear_cb),        self);
	g_signal_connect(eina_obj_get_lomo(self), "play",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "stop",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "pause",    G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "eos",      G_CALLBACK(lomo_eos_cb),          self);
	g_signal_connect(eina_obj_get_lomo(self), "all-tags", G_CALLBACK(lomo_all_tags_cb),     self);
	g_signal_connect(eina_obj_get_lomo(self), "error",    G_CALLBACK(lomo_error_cb),        self);

	gel_object_bind_mutual(
		(GObject *) eina_obj_get_lomo(self), "random",
		G_OBJECT(gel_ui_container_find_widget(GTK_CONTAINER(self->dock), "playlist-random-button")), "active");
	gel_object_bind_mutual(
		(GObject *) eina_obj_get_lomo(self), "repeat",
		G_OBJECT(gel_ui_container_find_widget(GTK_CONTAINER(self->dock), "playlist-repeat-button")), "active");

	// Accelerators
	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(EINA_OBJ_GET_WINDOW(self)), accel_group);
	gtk_widget_add_accelerator(eina_obj_get_typed(self, GTK_WIDGET, "playlist-search-entry"), "activate", accel_group,
		GDK_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	playlist_dnd_setup(self);

	gtk_widget_show(self->dock);
	return eina_dock_add_widget(GEL_APP_GET_DOCK(app), "Playlist",
		gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU), self->dock);
}

G_MODULE_EXPORT gboolean
playlist_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *self = GEL_APP_GET_PLAYLIST(app);
	gchar *file;
	gint i = 0;
	gint fd;

	gel_free_and_invalidate(self->io_op, NULL, gel_io_tree_op_close);
	g_queue_foreach(self->io_tree_files, (GFunc) g_object_unref, NULL);
	g_queue_clear(self->io_tree_files);

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
	eina_conf_set_int(EINA_OBJ_GET_SETTINGS(self), "/playlist/last_current", i);
	eina_dock_remove_widget(EINA_OBJ_GET_DOCK(self), "playlist");

	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

static gboolean
playlist_get_iter_from_stream(EinaPlaylist *self, GtkTreeIter *iter, LomoStream *stream)
{
	GtkTreeModel *model = gtk_tree_view_get_model(self->tv);
	g_return_val_if_fail(gtk_tree_model_get_iter_first(model, iter), FALSE);

	gboolean found = FALSE;
	LomoStream *s = NULL;
	do {
		gtk_tree_model_get(model, iter, PLAYLIST_COLUMN_STREAM, &s, -1);
		if (stream == s)
		{
			found = TRUE;
			break;
		}
	} while (gtk_tree_model_iter_next(model, iter));

	g_return_val_if_fail(found == TRUE, FALSE);
	return TRUE;
}

static void
playlist_set_valist_from_stream(EinaPlaylist *self, LomoStream *stream, ...)
{
	GtkTreeIter iter;
	g_return_if_fail(playlist_get_iter_from_stream(self, &iter, stream));

	va_list args;
	va_start(args, stream);
	gtk_list_store_set_valist((GtkListStore *) gtk_tree_view_get_model(self->tv), &iter, args);
	va_end(args);
}


static gboolean
dock_key_press_event_cb(GtkWidget *widget, GdkEvent  *event, EinaPlaylist *self) 
{
	if (event->type != GDK_KEY_PRESS)
		return FALSE;

	switch (event->key.keyval)
	{
	case GDK_q:
		playlist_queue_selected(self);
		break;

	case GDK_Delete:
		playlist_remove_selected(self);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

gboolean
eina_playlist_dock_init(EinaPlaylist *self)
{
	self->tv    = eina_obj_get_typed(self, GTK_TREE_VIEW, "playlist-treeview");
	self->model = eina_obj_get_typed(self, GTK_TREE_MODEL, "playlist-model");

	g_object_set(eina_obj_get_object(self, "title-renderer"),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_END,
		NULL);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->tv), GTK_SELECTION_MULTIPLE);

	self->dock = eina_obj_get_widget(self, "playlist-main-widget");
	g_object_ref(self->dock);
	if (gtk_widget_get_parent(self->dock) != NULL)
		gtk_widget_unparent(self->dock);

	g_signal_connect(self->dock, "key-press-event", G_CALLBACK(dock_key_press_event_cb), self);

	playlist_update_tab(self);

#if ENABLE_EXPERIMENTAL
	gtk_widget_show(eina_obj_get_widget(self, "search-separator"));
	gtk_widget_show(eina_obj_get_widget(self, "search-frame"));
#else
	gtk_widget_hide(eina_obj_get_widget(self, "search-separator"));
	gtk_widget_hide(eina_obj_get_widget(self, "search-frame"));
#endif

	return TRUE;
}

static gboolean
playlist_resize_columns(EinaPlaylist *self)
{
	static gint l[] = {-1, -1, -1};
	GtkAllocation alloc;
	gtk_widget_get_allocation((GtkWidget *) self->tv, &alloc);
	gint tv = alloc.width;
	gint sc = gtk_tree_view_column_get_width(eina_obj_get_typed((EinaObj *) self, GTK_TREE_VIEW_COLUMN, "state-column"));
	gint qc = gtk_tree_view_column_get_width(eina_obj_get_typed((EinaObj *) self, GTK_TREE_VIEW_COLUMN, "queue-column"));

	if ((l[0] == tv) && (l[1] == sc) && (l[2] == qc))
		return FALSE;
	l[0] = tv;
	l[1] = sc;
	l[2] = qc;

	gint out = tv - sc - qc - 4;
	gtk_cell_renderer_set_fixed_size(eina_obj_get_typed((EinaObj *) self, GTK_CELL_RENDERER, "title-renderer"), out, -1);
	gtk_tree_view_column_set_fixed_width(eina_obj_get_typed((EinaObj *) self, GTK_TREE_VIEW_COLUMN, "title-column"), out );

	gtk_tree_view_column_queue_resize(eina_obj_get_typed((EinaObj *) self, GTK_TREE_VIEW_COLUMN, "title-column"));
	return FALSE;
}

gint *
get_selected_indices(EinaPlaylist *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model = NULL;
	GList *rows, *l = NULL;

	// Get selected
	selection = gtk_tree_view_get_selection(self->tv);
	l = rows = gtk_tree_selection_get_selected_rows(selection, &model );

	// Create an integer list
	guint i = 0;
	gint *ret = g_new0(gint, g_list_length(rows)  + 1);
	while (l)
	{
		GtkTreeIter iter;
		guint       index;
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*  )(l->data));
		gtk_tree_model_get(model, &iter,
			PLAYLIST_COLUMN_INDEX, &index,
			-1);
		ret[i] = index;

		i++;
		l = l->next;
	}
	ret[i] = -1;

	// Free results
	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	return ret;
}

static void
playlist_queue_selected(EinaPlaylist *self)
{
	gint *indices = get_selected_indices(self);
	LomoPlayer *lomo = eina_obj_get_lomo(self);
	gint i = 0;
	for (i = 0; indices[i] != -1; i++)
	{
		LomoStream *stream = lomo_player_nth_stream(lomo, indices[i]);
		gint queue_index = lomo_player_queue_index(lomo, stream);
		if (queue_index == -1)
			lomo_player_queue_stream(lomo, stream);
		else
			lomo_player_dequeue(lomo, queue_index);
	}
	g_free(indices);
}

static void
playlist_dequeue_selected(EinaPlaylist *self)
{
	gint *indices = get_selected_indices(self);

	GtkTreeModel *model = gtk_tree_view_get_model(self->tv);

	// Remove in inverse order
	gint i = 0;
	for (i = 0; indices[i] != -1; i++)
	{
		GtkTreeIter iter;
		GtkTreePath *treepath = gtk_tree_path_new_from_indices(indices[i], -1);
		if (!gtk_tree_model_get_iter(model, &iter, treepath))
		{
			gel_warn("Cannot get iter for index %d", indices[i]);
			gtk_tree_path_free(treepath);
			continue;
		}
		gtk_tree_path_free(treepath);

		LomoStream *stream;
		gtk_tree_model_get(model, &iter, PLAYLIST_COLUMN_STREAM, &stream, -1);

		gint idx = lomo_player_queue_index(eina_obj_get_lomo(self), stream);
		if (idx == -1)
			gel_warn("Stream not in queue");
		else
			lomo_player_dequeue(eina_obj_get_lomo(self), idx);
	}
	g_free(indices);
}

static void
playlist_remove_selected(EinaPlaylist *self)
{
	LomoPlayer *lomo = eina_obj_get_lomo(self);
	gint *indices = get_selected_indices(self);
	gint i;

	for (i = 0; indices[i] != -1; i++);

	for (i = i - 1; i >= 0; i--)
	{
		gel_warn("Deleting #%d", indices[i]);
		lomo_player_del(lomo, indices[i]);
	}
	g_free(indices);
}

static void
playlist_update_tab(EinaPlaylist *self)
{
	GtkNotebook *nb  = eina_obj_get_typed(self, GTK_NOTEBOOK, "playlist-notebook");
	g_return_if_fail(nb != NULL);

	LomoPlayer *lomo = eina_obj_get_lomo(self);
	gint curr = gtk_notebook_get_current_page(nb);

	if (lomo == NULL)
	{
		gtk_notebook_set_current_page(nb, TAB_PLAYLIST_EMPTY);
		g_return_if_fail(lomo != NULL);
	}

	gint new_tab = lomo_player_get_total(lomo) ? TAB_PLAYLIST_NON_EMPTY : TAB_PLAYLIST_EMPTY;
	if (curr != new_tab)
	{
		gboolean sensitive = (new_tab == TAB_PLAYLIST_NON_EMPTY);
		gchar *widgets[] = { "playlist-random-button", "playlist-repeat-button", "playlist-remove-button", "playlist-clear-button", NULL};
		gint i;
		for (i = 0; widgets[i] != NULL; i++)
			gtk_widget_set_sensitive(eina_obj_get_typed(self, GTK_WIDGET, widgets[i]), sensitive);
		gtk_notebook_set_current_page(nb, new_tab);
	}
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
	gtk_list_store_set((GtkListStore *) self->model, iter,
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
		gtk_list_store_set((GtkListStore *) self->model, iter,
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
		// treepath = gtk_tree_model_get_path(self->model, iter);
		treepath = gtk_tree_model_get_path(self->model, iter);
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
		// gtk_tree_model_get_iter(self->model, &_iter, treepath);
		gtk_tree_model_get_iter(self->model, &_iter, treepath);
		gtk_tree_path_free(treepath);
		iter = &_iter;
	}

	current = lomo_player_get_current(eina_obj_get_lomo(self));

	// Get data from GtkListStore supported columns
	// gtk_tree_model_get(self->model, iter,
	gtk_tree_model_get(self->model, iter,
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

// ------------
// UI Callbacks
// ------------
static void
playlist_treeview_size_allocate_cb(GtkWidget *w, GtkAllocation *a, EinaPlaylist *self)
{
	static guint id = 0;
	if (id > 0)
		g_source_remove(id);
	id = g_idle_add((GSourceFunc) playlist_resize_columns, self);
}

void
dock_row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self)
{
	GError *err = NULL;

	GtkTreeModel *model = gtk_tree_view_get_model(self->tv);

	GtkTreeIter  iter;
	gtk_tree_model_get_iter(model, &iter, path);

	guint index;
	gtk_tree_model_get(model, &iter,
		PLAYLIST_COLUMN_INDEX, &index,
		-1);

	if (!lomo_player_go_nth(eina_obj_get_lomo(self), index, &err))
	{
		gel_error(N_("Cannot go to stream #%d: '%s'"), index, err->message);
		g_error_free(err);
		return;
	}

	if (lomo_player_get_state(eina_obj_get_lomo(self)) == LOMO_STATE_PLAY)
		return;

	if (!lomo_player_play(eina_obj_get_lomo(self), &err))
	{
		gel_error(N_("Cannot set state to LOMO_STATE_PLAY: '%s'"), err->message);
		g_error_free(err);
	}
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
		gchar *tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		tag = g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
	}

	return tag;
}

static gboolean
tree_model_filter_func_cb(GtkTreeModel *model, GtkTreeIter *iter, const gchar *query)
{
	gchar *title = NULL, *title_lc, *key_lc;
	gboolean ret = FALSE;

	gtk_tree_model_get(model, iter,
		PLAYLIST_COLUMN_MARKUP, &title,
		-1);
	title_lc = g_utf8_strdown(title, -1);
	key_lc = g_utf8_strdown(query, -1);
	g_free(title);

	if (strstr(title_lc, key_lc) != NULL)
		ret = TRUE;
	else
		ret = FALSE;
	g_free(title_lc);
	g_free(key_lc);

	return ret;
}

static void search_entry_changed_cb
(GtkWidget *w, EinaPlaylist *self)
{
	const gchar *query = gtk_entry_get_text((GtkEntry *) w);
	GObject *model = (GObject *) gtk_tree_view_get_model(self->tv);

	gboolean got_query = (query && query[0]);

	if (!got_query && GTK_IS_TREE_MODEL_FILTER(model))
	{
		gtk_tree_view_set_model(self->tv, self->model);
		g_object_unref(model);
	}

	if (got_query && !GTK_IS_TREE_MODEL_FILTER(model))
	{
		GtkTreeModel *filter = gtk_tree_model_filter_new(self->model, NULL);
		gtk_tree_view_set_model(self->tv, filter);
		gtk_tree_model_filter_set_visible_func((GtkTreeModelFilter *) filter, (GtkTreeModelFilterVisibleFunc) tree_model_filter_func_cb, (gpointer) query, NULL);
	}

	if (got_query)
	{
		GtkTreeModelFilter *filter = (GtkTreeModelFilter *) gtk_tree_view_get_model(self->tv);
		gtk_tree_model_filter_refilter(filter);
	}
}

static void search_entry_icon_press_cb
(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, EinaPlaylist *self)
{
	if (icon_pos != GTK_ENTRY_ICON_SECONDARY)
		return;
	gtk_entry_set_text(entry, "");
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
	gtk_list_store_insert_with_values((GtkListStore *) self->model,
		&iter, pos,
		PLAYLIST_COLUMN_INDEX,  pos,
		PLAYLIST_COLUMN_STREAM, stream,
		PLAYLIST_COLUMN_STATE,  NULL,
		PLAYLIST_COLUMN_TEXT,   v,
		PLAYLIST_COLUMN_MARKUP, m ? m : v,
		-1);
	g_free(v);
	gel_free_and_invalidate(m, NULL, g_free);

	playlist_update_tab(self);
}

static void lomo_remove_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	GtkTreeModel *model   = self->model;
	GtkTreePath *treepath = gtk_tree_path_new_from_indices(pos, -1);
	GtkTreeIter  iter;

	// Get iter
	if (!gtk_tree_model_get_iter(model, &iter, treepath))
	{
		gtk_tree_path_free(treepath);
		gel_error("Cannot find iter corresponding to index #%d", pos);
		return;
	}
	gtk_tree_path_free(treepath);

	// Delete element
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

	// Update indices from pos to bottom
	guint i;
	guint total = lomo_player_get_total(lomo);
	treepath = gtk_tree_path_new_from_indices(pos, -1);
	gtk_tree_model_get_iter(model, &iter, treepath);

	for (i = pos; i < total; i++)
	{
		gtk_list_store_set(GTK_LIST_STORE(self->model), &iter,
			PLAYLIST_COLUMN_INDEX, i,
			-1);
		  gtk_tree_model_iter_next(model, &iter);
	}
	gtk_tree_path_free(treepath);

	playlist_update_tab(self);
}

static void lomo_queue_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	gchar *queue_str = g_strdup_printf("<b>%d</b>", pos + 1);
	playlist_set_valist_from_stream(self, stream, PLAYLIST_COLUMN_QUEUE_STR, queue_str, -1); 
	g_free(queue_str);
}

static void lomo_dequeue_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	playlist_set_valist_from_stream(self, stream, PLAYLIST_COLUMN_QUEUE_STR, NULL, -1);

	while ((stream = lomo_player_queue_nth(lomo, pos)) != NULL)
	{
		gchar *str = g_strdup_printf("<b>%d</b>", pos + 1);
		playlist_set_valist_from_stream(self, stream, PLAYLIST_COLUMN_QUEUE_STR, str, -1);
		g_free(str);
		pos++;
	}
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
	gtk_list_store_clear((GtkListStore *) self->model);
	playlist_update_tab(self);
}

static gboolean
lomo_eos_cb_helper(LomoPlayer *lomo)
{
	if (lomo_player_get_next(lomo) != -1)
	{
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

	if (!gtk_tree_model_get_iter(self->model, &iter, treepath))
	{
		gel_warn("Cannot get iter for stream %p", stream);
		gtk_tree_path_free(treepath);
		return;
	}
	gtk_tree_path_free(treepath);
		
	gtk_list_store_set((GtkListStore *) self->model, &iter,
		PLAYLIST_COLUMN_STATE, GTK_STOCK_STOP,
		-1);

	lomo_player_go_next(lomo, NULL);
	lomo_player_play(lomo, NULL);
}

static gboolean
widget_focus_in_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self)
{
	eina_window_key_bindings_set_enabled(EINA_OBJ_GET_WINDOW(self), FALSE);
	return FALSE;
}

static gboolean
widget_focus_out_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self)
{
	eina_window_key_bindings_set_enabled(EINA_OBJ_GET_WINDOW(self), TRUE);
	return FALSE;
}

static void lomo_all_tags_cb
(LomoPlayer *lomo, LomoStream *stream, EinaPlaylist *self)
{
	GtkTreeModel *model = self->model;
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

gboolean treeview_button_press_event_cb
(GtkWidget *w, GdkEventButton *ev, EinaPlaylist *self)
{
	if ((ev->type != GDK_BUTTON_PRESS) || (ev->button != 3))
		return FALSE;

	GtkMenu *menu = eina_obj_get_typed(self, GTK_MENU, "popup-menu");
	g_return_val_if_fail(menu != NULL, FALSE);
	
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, ev->button,  gtk_get_current_event_time());

	return TRUE;
}

void action_activate_cb
(GtkAction *action, EinaPlaylist *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal("add-action", name))
		eina_fs_load_from_default_file_chooser(eina_obj_get_app(self));

	else if (g_str_equal("remove-action", name))
		playlist_remove_selected(self);
	
	else if (g_str_equal("clear-action", name))
		lomo_player_clear(eina_obj_get_lomo(self));

	else if (g_str_equal("queue-action", name))
		playlist_queue_selected(self);

	else if (g_str_equal("dequeue-action", name))
		playlist_dequeue_selected(self);

	else
		gel_warn("Unknow action %s", name);
}

static void
io_tree_read_success_cb(GelIOTreeOp *op, const GFile *source, const GNode *result, EinaPlaylist *self)
{
	GList *l = gel_io_tree_result_flatten(result);
	GList *i = l;
	while (i)
	{
		GFile     *file = G_FILE(i->data);
		GFileInfo *info = g_object_get_data((GObject *) i->data, "gfileinfo");
		gchar     *uri = g_file_get_uri(file);
		if ((eina_file_utils_is_supported_extension(uri)) && (g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR))
			lomo_player_append_uri(eina_obj_get_lomo(self), uri);
		g_free(uri);
		i = i->next;
	}

	gel_io_tree_op_close(op);
	g_list_free(l);

	if (g_queue_is_empty(self->io_tree_files))
	{
		self->io_op = NULL;
		return;
	}

	self->io_op = gel_io_tree_walk(g_queue_pop_head(self->io_tree_files), "standard::*", TRUE,
		(GelIOTreeSuccessFunc) io_tree_read_success_cb, io_tree_read_error_cb, self);
}

static void
io_tree_read_error_cb(GelIOTreeOp *op, const GFile *source, const GError *error, gpointer data)
{
	gchar *uri = g_file_get_uri((GFile *) source);
	gel_warn(N_("Error on file %s: %s"), uri, error->message);
	g_free(uri);
}

// ---
// DnD
// ---
enum {
	DND_TARGET_STRING
};

static GtkTargetEntry dnd_target_list[] = {
	{ "STRING",     0, DND_TARGET_STRING },
	{ "text/plain", 0, DND_TARGET_STRING }
};

static guint dnd_n_targets = G_N_ELEMENTS(dnd_target_list);

static void
drag_data_received_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
        GtkSelectionData *selection_data, guint target_type, guint time,
	EinaPlaylist *self)
{
	gchar   *_sdata;

	gboolean dnd_success = FALSE;

	if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
	{
		if (target_type == DND_TARGET_STRING)
		{
			_sdata = (gchar*) gtk_selection_data_get_data(selection_data);
			dnd_success = TRUE;
		}
		else
				gel_warn("Unknow DnD type");
	}

	if (dnd_success == FALSE)
		gel_error("DnD data transfer failed!\n");
	else
	{
		gchar **uris = g_uri_list_extract_uris(_sdata);
		gint i;
		for (i = 0; uris[i] && uris[i][0] ; i++)
			g_queue_push_tail(self->io_tree_files, g_file_new_for_uri(uris[i]));
		
		g_strfreev(uris);
	}

	gtk_drag_finish (context, dnd_success, FALSE, time);

	if (g_queue_is_empty(self->io_tree_files))
		return;

	if (self->io_op)
	{
		gel_io_tree_op_close(self->io_op);
		self->io_op = NULL;
	}
	
	self->io_op = gel_io_tree_walk(g_queue_pop_head(self->io_tree_files), "standard::*", TRUE,
		(GelIOTreeSuccessFunc) io_tree_read_success_cb, io_tree_read_error_cb, self);
}

/* Emitted when a drag is over the destination */
static gboolean
drag_motion_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t,
	gpointer user_data)
{
	return  FALSE;
}

/* Emitted when a drag leaves the destination */
static void
drag_leave_cb
(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data)
{
}

static gboolean
drag_drop_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time,
	gpointer user_data)
{
	gboolean        is_valid_drop_site;
	GdkAtom         target_type;

	/* Check to see if (x,y) is a valid drop site within widget */
	is_valid_drop_site = TRUE;

	/* If the source offers a target */
	if (context-> targets)
	{
		/* Choose the best target type */
		target_type = GDK_POINTER_TO_ATOM(g_list_nth_data (context->targets, DND_TARGET_STRING));

		/* Request the data from the source. */
		gtk_drag_get_data
		(
			widget,         /* will receive 'drag-data-received' signal */
			context,        /* represents the current state of the DnD */
			target_type,    /* the target type we want */
			time            /* time stamp */
		);
	}

	/* No target offered by source => error */
	else
	{
		is_valid_drop_site = FALSE;
	}

	return  is_valid_drop_site;
}

static void
playlist_dnd_setup(EinaPlaylist *self)
{
	GtkWidget *dest = eina_obj_get_widget((EinaObj*) self, "playlist-treeview");
	gtk_drag_dest_set
	(
		dest,                        /* widget that will accept a drop */
		GTK_DEST_DEFAULT_DROP        /* default actions for dest on DnD */
		| GTK_DEST_DEFAULT_MOTION 
		| GTK_DEST_DEFAULT_HIGHLIGHT,
		dnd_target_list,             /* lists of target to support */
		dnd_n_targets,               /* size of list */
		GDK_ACTION_COPY              /* what to do with data after dropped */
	);

	/* All possible destination signals */
	g_signal_connect (dest, "drag-data-received", G_CALLBACK(drag_data_received_cb), self);
	g_signal_connect (dest, "drag-leave",         G_CALLBACK (drag_leave_cb),        self);
	g_signal_connect (dest, "drag-motion",        G_CALLBACK (drag_motion_cb),       self);
	g_signal_connect (dest, "drag-drop",          G_CALLBACK (drag_drop_cb),         self);
}


#if 0
#if 0

void drag_data_received_cb(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist       *self) {
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
}
#endif

void drag_data_get_cb(GtkWidget *w,
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
	EinaPlaylist     *self)
{
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
#endif

/*
 * Connector
 */
EINA_PLUGIN_INFO_SPEC(playlist,
	NULL,                           // version
	"dock,lomo,settings,window",	// deps
	NULL,                           // author
	NULL,                           // url
	N_("Build-in playlist plugin"), // short
	NULL,                           // long
	NULL                            // icon
);

