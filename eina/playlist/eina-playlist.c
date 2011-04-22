/*
 * eina/playlist/eina-playlist.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-playlist.h"
#include "eina-playlist-ui.h"
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

G_DEFINE_TYPE (EinaPlaylist, eina_playlist, GEL_UI_TYPE_GENERIC)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLAYLIST, EinaPlaylistPrivate))

typedef struct _EinaPlaylistPrivate EinaPlaylistPrivate;

struct _EinaPlaylistPrivate {
	// Props.
	LomoPlayer *lomo;
	gchar *stream_mrkp;

	// Internals
	GtkTreeView  *tv;
	GtkTreeModel *model; // Model, tv has a filter attached

	GBinding *bind_random, *bind_repeat;
};

/*
 * Some enums
 */
enum {
	PROP_LOMO_PLAYER = 1,
	PROP_STREAM_MARKUP
};
#define PROP_STREAM_MARKUP_DEFAULT "{%a - }%t"

enum {
	ACTION_ACTIVATED,
	LAST_SIGNAL
};
guint eina_playlist_signals[LAST_SIGNAL] = { 0 };

enum {
	TAB_PLAYLIST_EMPTY = 0,
	TAB_PLAYLIST_NON_EMPTY
};

enum
{
	PLAYLIST_COLUMN_STREAM = 0,
	PLAYLIST_COLUMN_STATE,
	PLAYLIST_COLUMN_TEXT,
	PLAYLIST_COLUMN_MARKUP,
	PLAYLIST_COLUMN_INDEX,
	PLAYLIST_COLUMN_QUEUE_STR,
};

/*
 * Internal API
 */
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
static gchar *
format_stream(LomoStream *stream, gchar *fmt);
static gchar*
format_stream_cb(gchar key, LomoStream *stream);

void
eina_playlist_update_item(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...);
#define eina_playlist_update_current_item(self,...) \
	eina_playlist_update_item(self, NULL,lomo_player_get_current(GET_PRIVATE(self)->lomo),__VA_ARGS__)

/*
 * UI Callbacks
 */
static gboolean
key_press_event_cb(GtkWidget *widget, GdkEvent  *event, EinaPlaylist *self);
void
dock_row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self);
gboolean
treeview_button_press_event_cb(GtkWidget *w, GdkEventButton *ev, EinaPlaylist *self);
void
action_activate_cb(GtkAction *action, EinaPlaylist *self);

/*
 * Lomo callbacks
 */
static void lomo_notify_state_cb
(LomoPlayer *lomo, GParamSpec *pspec, EinaPlaylist *self);
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
eina_playlist_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_STREAM_MARKUP:
		g_value_set_string(value, eina_playlist_get_stream_markup((EinaPlaylist *) object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_playlist_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_LOMO_PLAYER:
		eina_playlist_set_lomo_player((EinaPlaylist *) object, g_value_get_object(value));
		break;
	case PROP_STREAM_MARKUP:
		eina_playlist_set_stream_markup((EinaPlaylist *) object, (gchar *) g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_playlist_dispose (GObject *object)
{
	EinaPlaylist *self = EINA_PLAYLIST(object);
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);
	gel_free_and_invalidate(priv->stream_mrkp, NULL, g_free);

	G_OBJECT_CLASS (eina_playlist_parent_class)->dispose (object);
}

static void
eina_playlist_class_init (EinaPlaylistClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPlaylistPrivate));

	object_class->get_property = eina_playlist_get_property;
	object_class->set_property = eina_playlist_set_property;
	object_class->dispose = eina_playlist_dispose;

	g_object_class_install_property(object_class, PROP_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "lomo-player", "lomo-player",
			LOMO_TYPE_PLAYER, G_PARAM_WRITABLE));

	g_object_class_install_property(object_class, PROP_STREAM_MARKUP,
		g_param_spec_string("stream-markup", "stream-markup", "stream-markup",
			PROP_STREAM_MARKUP_DEFAULT, G_PARAM_WRITABLE | G_PARAM_READABLE));

	eina_playlist_signals[ACTION_ACTIVATED] = 
		g_signal_new("action-activated",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (EinaPlaylistClass, action_activated),
			NULL, NULL,
			gel_marshal_BOOLEAN__OBJECT,
			G_TYPE_BOOLEAN,
			1,
			GTK_TYPE_ACTION);
}

static void
eina_playlist_init (EinaPlaylist *self)
{
	gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
}

EinaPlaylist*
eina_playlist_new (void)
{
	static struct {
		gchar     *object;
		gchar     *signal;
		GCallback  callback;
	} signals[] = 
	{
		{ "playlist-treeview",   "row-activated", G_CALLBACK(dock_row_activated_cb)              },
		{ "playlist-treeview",   "size-allocate", G_CALLBACK(playlist_treeview_size_allocate_cb) },
 
		{ "playlist-search-entry", "changed",         G_CALLBACK(search_entry_changed_cb)    },
		{ "playlist-search-entry", "icon-press",      G_CALLBACK(search_entry_icon_press_cb) },
		{ "playlist-search-entry", "activate",        G_CALLBACK(gtk_widget_grab_focus)      },
		{ "playlist-search-entry", "focus-in-event",  G_CALLBACK(widget_focus_in_event_cb)   },
		{ "playlist-search-entry", "focus-out-event", G_CALLBACK(widget_focus_out_event_cb)  },
	
		{ "playlist-treeview",     "button-press-event", G_CALLBACK(treeview_button_press_event_cb) }
	};
	static gchar *actions[] =
	{
		"random-action",
		"repeat-action",
		"add-action",
		"remove-action",
		"clear-action",
		"queue-action",
		"dequeue-action",
	};

 	EinaPlaylist *self = g_object_new (EINA_TYPE_PLAYLIST, "xml-string", __ui_xml, NULL);

 	EinaPlaylistPrivate *priv = GET_PRIVATE(self);
	priv->tv    = gel_ui_generic_get_typed(GEL_UI_GENERIC(self), GTK_TREE_VIEW,  "playlist-treeview");
	priv->model = gel_ui_generic_get_typed(GEL_UI_GENERIC(self), GTK_TREE_MODEL, "playlist-model");

	g_object_set(
		gel_ui_generic_get_typed(GEL_UI_GENERIC(self), G_OBJECT, "title-renderer"),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_END,
		NULL);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(priv->tv), GTK_SELECTION_MULTIPLE);

	for (guint i = 0; i < G_N_ELEMENTS(signals); i++)
	{
		GObject *o = gel_ui_generic_get_object((GelUIGeneric *) self, signals[i].object);
		if (!o || !G_IS_OBJECT(o))
			g_warning(N_("Missing object '%s'"), signals[i].object);
		else
			g_signal_connect(gel_ui_generic_get_object((GelUIGeneric *) self, signals[i].object), signals[i].signal, signals[i].callback, self);
	}
	for (guint i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GObject *a = gel_ui_generic_get_object((GelUIGeneric *) self, actions[i]);
		if (!a || !GTK_IS_ACTION(a))
			g_warning(N_("Missing action '%s'"), actions[i]);
		else
			g_signal_connect(a, "activate", (GCallback) action_activate_cb, self);
	}
	g_signal_connect(self, "key-press-event", G_CALLBACK(key_press_event_cb), self);

#if ENABLE_EXPERIMENTAL
	gtk_widget_show(gel_ui_generic_get_widget(self, "search-separator"));
	gtk_widget_show(gel_ui_generic_get_widget(self, "search-frame"));
#else
	gtk_widget_hide(gel_ui_generic_get_widget(self, "search-separator"));
	gtk_widget_hide(gel_ui_generic_get_widget(self, "search-frame"));
#endif

	return self;
}

void
eina_playlist_set_lomo_player(EinaPlaylist *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	const struct {
		gchar *signal;
		GCallback callback;	
	} callback_defs[] = {
		{ "insert",   G_CALLBACK(lomo_insert_cb)       },
		{ "remove",   G_CALLBACK(lomo_remove_cb)       },
		{ "queue",    G_CALLBACK(lomo_queue_cb)        },
		{ "dequeue",  G_CALLBACK(lomo_dequeue_cb)      },
		{ "change",   G_CALLBACK(lomo_change_cb)       },
		{ "clear",    G_CALLBACK(lomo_clear_cb)        },
		{ "notify::state", G_CALLBACK(lomo_notify_state_cb) },
		{ "eos",      G_CALLBACK(lomo_eos_cb)          },
		{ "all-tags", G_CALLBACK(lomo_all_tags_cb)     },
		{ "error",    G_CALLBACK(lomo_error_cb)        }
	};

	if (priv->lomo)
	{
		for (guint i = 0; i < G_N_ELEMENTS(callback_defs); i++)
			g_signal_handlers_disconnect_by_func(priv->lomo, callback_defs[i].callback, self);
		g_object_unref(priv->bind_random);
		g_object_unref(priv->bind_repeat);
		g_object_unref(priv->lomo);
	}

	priv->lomo = g_object_ref(lomo);

	priv->bind_repeat = g_object_bind_property(lomo, "repeat", gel_ui_generic_get_object((GelUIGeneric *) self, "repeat-action"), "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	priv->bind_random = g_object_bind_property(lomo, "random", gel_ui_generic_get_object((GelUIGeneric *) self, "random-action"), "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	for (guint i = 0; i < G_N_ELEMENTS(callback_defs); i++)
		g_signal_connect(priv->lomo, callback_defs[i].signal, callback_defs[i].callback, self);

	lomo_clear_cb(priv->lomo, self);
	gint i = 0;
	GList *items = (GList *) lomo_player_get_playlist(priv->lomo);
	while (items)
	{
		lomo_insert_cb(priv->lomo, LOMO_STREAM(items->data), i, self);
		i++;
		items = items->next;
	}

	g_object_notify((GObject *) self, "lomo-player");
}

void
eina_playlist_set_stream_markup(EinaPlaylist *self, gchar *markup)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(markup != NULL);

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);
	gel_free_and_invalidate(priv->stream_mrkp, NULL, g_free);
	priv->stream_mrkp = g_strdup(markup);

	g_object_notify((GObject *) self, "stream-markup");
}

gchar*
eina_playlist_get_stream_markup(EinaPlaylist *self)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), NULL);
	return g_strdup(GET_PRIVATE(self)->stream_mrkp);
}

/*
// Keybindings
static EinaWindowKeyBind keybindings[] = {
	{ GDK_Control_L | GDK_f, (EinaWindowKeyBindHandler) window_keybind_handler },
	{ GDK_Control_R | GDK_f, (EinaWindowKeyBindHandler) window_keybind_handler } 
};
*/
#if 0
G_MODULE_EXPORT gboolean
playlist_plugin_init (GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlaylist *self;

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


	// Accelerators
	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(EINA_OBJ_GET_WINDOW(self)), accel_group);
	gtk_widget_add_accelerator(gel_ui_generic_get_typed(self, GTK_WIDGET, "playlist-search-entry"), "activate", accel_group,
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

	GList *iter  = (GList *) lomo_player_get_playlist(eina_obj_get_lomo(self));
	char **out = g_new0(gchar*, g_list_length(iter) + 1);
	while (iter)
	{
		out[i++] = lomo_stream_get_tag(LOMO_STREAM(iter->data), LOMO_TAG_URI);
		iter = iter->next;
	}
	
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

	g_free(self->stream_fmt);
	eina_dock_remove_widget(EINA_OBJ_GET_DOCK(self), "playlist");

	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}
#endif

static gboolean
playlist_get_iter_from_stream(EinaPlaylist *self, GtkTreeIter *iter, LomoStream *stream)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), FALSE);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), FALSE);

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	GtkTreeModel *model = gtk_tree_view_get_model(priv->tv);
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	GtkTreeIter iter;
	g_return_if_fail(playlist_get_iter_from_stream(self, &iter, stream));

	va_list args;
	va_start(args, stream);
	gtk_list_store_set_valist((GtkListStore *) gtk_tree_view_get_model(GET_PRIVATE(self)->tv), &iter, args);
	va_end(args);
}


static gboolean
key_press_event_cb(GtkWidget *widget, GdkEvent  *event, EinaPlaylist *self) 
{
	if (event->type != GDK_KEY_PRESS)
		return FALSE;

	switch (event->key.keyval)
	{
	case GDK_KEY_q:
		playlist_queue_selected(self);
		break;

	case GDK_KEY_Delete:
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
	return TRUE;
}

static gboolean
playlist_resize_columns(EinaPlaylist *self)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), FALSE);
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	static gint l[] = {-1, -1, -1};
	GtkAllocation alloc;
	gtk_widget_get_allocation((GtkWidget *) priv->tv, &alloc);
	gint tv = alloc.width;
	gint sc = gtk_tree_view_column_get_width(gel_ui_generic_get_typed(self, GTK_TREE_VIEW_COLUMN, "state-column"));
	gint qc = gtk_tree_view_column_get_width(gel_ui_generic_get_typed(self, GTK_TREE_VIEW_COLUMN, "queue-column"));

	if ((l[0] == tv) && (l[1] == sc) && (l[2] == qc))
		return FALSE;
	l[0] = tv;
	l[1] = sc;
	l[2] = qc;

	gint out = tv - sc - qc - 4;
	gtk_cell_renderer_set_fixed_size(gel_ui_generic_get_typed(self, GTK_CELL_RENDERER, "title-renderer"), out, -1);
	gtk_tree_view_column_set_fixed_width(gel_ui_generic_get_typed(self, GTK_TREE_VIEW_COLUMN, "title-column"), out );

	gtk_tree_view_column_queue_resize(gel_ui_generic_get_typed(self, GTK_TREE_VIEW_COLUMN, "title-column"));
	return FALSE;
}

gint *
get_selected_indices(EinaPlaylist *self)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), NULL);
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	GtkTreeSelection *selection;
	GtkTreeModel     *model = NULL;
	GList *rows, *l = NULL;

	// Get selected
	selection = gtk_tree_view_get_selection(priv->tv);
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gint *indices = get_selected_indices(self);
	gint i = 0;
	for (i = 0; indices[i] != -1; i++)
	{
		LomoStream *stream = lomo_player_nth_stream(priv->lomo, indices[i]);
		gint queue_index = lomo_player_queue_index(priv->lomo, stream);
		if (queue_index == -1)
			lomo_player_queue_stream(priv->lomo, stream);
		else
			lomo_player_dequeue(priv->lomo, queue_index);
	}
	g_free(indices);
}

static void
playlist_dequeue_selected(EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gint *indices = get_selected_indices(self);

	GtkTreeModel *model = gtk_tree_view_get_model(priv->tv);

	// Remove in inverse order
	gint i = 0;
	for (i = 0; indices[i] != -1; i++)
	{
		GtkTreeIter iter;
		GtkTreePath *treepath = gtk_tree_path_new_from_indices(indices[i], -1);
		if (!gtk_tree_model_get_iter(model, &iter, treepath))
		{
			g_warning(N_("Cannot get iter for index %d"), indices[i]);
			gtk_tree_path_free(treepath);
			continue;
		}
		gtk_tree_path_free(treepath);

		LomoStream *stream;
		gtk_tree_model_get(model, &iter, PLAYLIST_COLUMN_STREAM, &stream, -1);

		gint idx = lomo_player_queue_index(priv->lomo, stream);
		if (idx == -1)
			g_warning(N_("Stream not in queue"));
		else
			lomo_player_dequeue(priv->lomo, idx);
	}
	g_free(indices);
}

static void
playlist_remove_selected(EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gint *indices = get_selected_indices(self);
	gint i;

	for (i = 0; indices[i] != -1; i++);

	for (i = i - 1; i >= 0; i--)
		lomo_player_del(priv->lomo, indices[i]);

	g_free(indices);
}

static void
playlist_update_tab(EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	GtkNotebook *nb  = gel_ui_generic_get_typed(self, GTK_NOTEBOOK, "playlist-notebook");
	g_return_if_fail(nb != NULL);

	gint curr = gtk_notebook_get_current_page(nb);

	if (priv->lomo == NULL)
	{
		gtk_notebook_set_current_page(nb, TAB_PLAYLIST_EMPTY);
		g_return_if_fail(priv->lomo != NULL);
	}

	gint new_tab = lomo_player_get_total(priv->lomo) ? TAB_PLAYLIST_NON_EMPTY : TAB_PLAYLIST_EMPTY;
	if (curr != new_tab)
	{
		gboolean sensitive = (new_tab == TAB_PLAYLIST_NON_EMPTY);
		gchar *widgets[] = { "playlist-random-button", "playlist-repeat-button", "playlist-remove-button", "playlist-clear-button", NULL};
		gint i;
		for (i = 0; widgets[i] != NULL; i++)
			gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, widgets[i]), sensitive);
		gtk_notebook_set_current_page(nb, new_tab);
	}
}


void
__eina_playlist_update_item_state(EinaPlaylist *self, GtkTreeIter *iter, gint item, gchar *current_state, gint current_item)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	LomoState state = LOMO_STATE_INVALID;
	gchar *new_state = NULL;

	// If item is not the active one, new_state is NULL
	if (item == current_item)
	{
		state = lomo_player_get_state(priv->lomo);
		switch (state)
		{
			case LOMO_STATE_INVALID:
				// leave stock-id as NULL
				break;
			case LOMO_STATE_PLAY:
				 if (gtk_widget_get_direction((GtkWidget *) self) == GTK_TEXT_DIR_LTR)
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
				g_warning(N_("Unknow state: %d"), state);
				break;
		}
	}

	// Update only if needed
	if ((current_state != NULL) && (new_state != NULL))
		if (g_str_equal(new_state, current_state))
			return;

	gtk_list_store_set((GtkListStore *) priv->model, iter,
		PLAYLIST_COLUMN_STATE, new_state,
		-1);
}

void
__eina_playlist_update_item_title(EinaPlaylist *self, GtkTreeIter *iter, gint item, gchar *current_title, gint current_item, gchar *current_raw_title)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gchar *markup = NULL;

	// Calculate new value
	if (item == current_item)
		markup = g_strdup_printf("<b>%s</b>", current_raw_title);
	else
		markup = g_strdup(current_raw_title);
	
	// Update only if needed
	if (!g_str_equal(current_title, markup))
		gtk_list_store_set((GtkListStore *) priv->model, iter,
			PLAYLIST_COLUMN_MARKUP, markup,
			-1);
	g_free(markup);
}

void
eina_playlist_update_item(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

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
		g_warning(N_("You must supply an iter _or_ an item"));
		return;
	}

	// Get item from iter
	if (item == -1)
	{
		treepath = gtk_tree_model_get_path(priv->model, iter);
		if (treepath == NULL)
		{
			g_warning(N_("Cannot get a GtkTreePath from index %d"), item);
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
		g_return_if_fail(gtk_tree_model_get_iter(priv->model, &_iter, treepath));
		gtk_tree_path_free(treepath);
		iter = &_iter;
	}

	current = lomo_player_get_current(priv->lomo);

	// Get data from GtkListStore supported columns
	gtk_tree_model_get(priv->model, iter,
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
				g_warning(N_("Unknow column %d"), column);
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	GError *err = NULL;

	GtkTreeModel *model = gtk_tree_view_get_model(priv->tv);

	GtkTreeIter  iter;
	gtk_tree_model_get_iter(model, &iter, path);

	guint index;
	gtk_tree_model_get(model, &iter,
		PLAYLIST_COLUMN_INDEX, &index,
		-1);

	if (!lomo_player_go_nth(priv->lomo, index, &err))
	{
		g_warning(N_("Cannot go to stream #%d: '%s'"), index, err->message);
		g_error_free(err);
		return;
	}

	if (lomo_player_get_state(priv->lomo) == LOMO_STATE_PLAY)
		return;

	if (!lomo_player_play(priv->lomo, &err))
	{
		g_warning(N_("Cannot set state to LOMO_STATE_PLAY: '%s'"), err->message);
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	const gchar *query = gtk_entry_get_text((GtkEntry *) w);
	GObject *model = (GObject *) gtk_tree_view_get_model(priv->tv);

	gboolean got_query = (query && query[0]);

	if (!got_query && GTK_IS_TREE_MODEL_FILTER(model))
	{
		gtk_tree_view_set_model(priv->tv, priv->model);
		g_object_unref(model);
	}

	if (got_query && !GTK_IS_TREE_MODEL_FILTER(model))
	{
		GtkTreeModel *filter = gtk_tree_model_filter_new(priv->model, NULL);
		gtk_tree_view_set_model(priv->tv, filter);
		gtk_tree_model_filter_set_visible_func((GtkTreeModelFilter *) filter, (GtkTreeModelFilterVisibleFunc) tree_model_filter_func_cb, (gpointer) query, NULL);
	}

	if (got_query)
	{
		GtkTreeModelFilter *filter = (GtkTreeModelFilter *) gtk_tree_view_get_model(priv->tv);
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
static void lomo_notify_state_cb
(LomoPlayer *lomo, GParamSpec *pspec, EinaPlaylist *self)
{
	g_return_if_fail(g_str_equal(pspec->name, "state"));
	eina_playlist_update_current_item(self, PLAYLIST_COLUMN_STATE, -1);
}

static void lomo_insert_cb
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gchar *text = format_stream(stream, priv->stream_mrkp);
	gchar *v = g_markup_escape_text(text, -1);
	g_free(text);

	gchar *m = NULL;
	if (pos == lomo_player_get_current(lomo))
		m = g_strdup_printf("<b>%s</b>", v);

	GtkTreeIter iter;
	gtk_list_store_insert_with_values((GtkListStore *) priv->model,
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	GtkTreeModel *model   = priv->model;
	GtkTreePath *treepath = gtk_tree_path_new_from_indices(pos, -1);
	GtkTreeIter  iter;

	// Get iter
	if (!gtk_tree_model_get_iter(model, &iter, treepath))
	{
		gtk_tree_path_free(treepath);
		g_warning(N_("Cannot find iter corresponding to index #%d"), pos);
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
		gtk_list_store_set(GTK_LIST_STORE(priv->model), &iter,
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail((old >= 0) || (new >= 0));

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

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
		gtk_tree_view_scroll_to_cell(priv->tv, treepath, NULL,
			TRUE, 0.5, 0.0);
		gtk_tree_path_free(treepath);
	}
}

static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));

	gtk_list_store_clear((GtkListStore *) GET_PRIVATE(self)->model);
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
	else
	{
		lomo_player_stop(lomo, NULL);
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
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));
	if (stream)
		g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	g_warning(N_("Got error on stream %p: %s"), stream, error->message);

	if ((stream == NULL) || !lomo_stream_get_failed_flag(stream))
		return;

	gint pos;
	if ((pos = lomo_player_index(lomo, stream)) == -1)
	{
		g_warning(N_("Stream %p doest belongs to Player %p"), stream, lomo);
		return;
	}

	GtkTreeIter iter;
	GtkTreePath *treepath = gtk_tree_path_new_from_indices(pos, -1);

	if (!gtk_tree_model_get_iter(priv->model, &iter, treepath))
	{
		g_warning(N_("Cannot get iter for stream %p"), stream);
		gtk_tree_path_free(treepath);
		return;
	}
	gtk_tree_path_free(treepath);
		
	gtk_list_store_set((GtkListStore *) priv->model, &iter,
		PLAYLIST_COLUMN_STATE, GTK_STOCK_STOP,
		-1);

	lomo_player_go_next(priv->lomo, NULL);
	lomo_player_play(priv->lomo, NULL);
}

static gboolean
widget_focus_in_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self)
{
	// eina_window_key_bindings_set_enabled(EINA_OBJ_GET_WINDOW(self), FALSE);
	return FALSE;
}

static gboolean
widget_focus_out_event_cb(GtkWidget *w, GdkEventFocus *ev, EinaPlaylist *self)
{
	// eina_window_key_bindings_set_enabled(EINA_OBJ_GET_WINDOW(self), TRUE);
	return FALSE;
}

static void lomo_all_tags_cb
(LomoPlayer *lomo, LomoStream *stream, EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	GtkTreeModel *model = priv->model;
	gint index = lomo_player_index(lomo, stream);
	if (index < 0)
	{
		g_warning(N_("Cannot find index for stream %p (%s)"), stream, lomo_stream_get_tag(stream, LOMO_TAG_URI));
		return;
	}

	GtkTreePath *treepath = gtk_tree_path_new_from_indices(index, -1);
	GtkTreeIter iter;
	if (!treepath || !gtk_tree_model_get_iter(model, &iter, treepath))
	{
		g_warning(N_("Cannot get iter for index %d"), index);
		return;
	}
	gtk_tree_path_free(treepath);

	gchar *text = format_stream(stream, priv->stream_mrkp);
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

	GtkMenu *menu = gel_ui_generic_get_typed(self, GTK_MENU, "popup-menu");
	g_return_val_if_fail(menu != NULL, FALSE);
	
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, ev->button,  gtk_get_current_event_time());

	return TRUE;
}

void action_activate_cb
(GtkAction *action, EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(GTK_IS_ACTION(action));

	EinaPlaylistPrivate *priv = GET_PRIVATE(self);

	gboolean ret = FALSE;
	g_signal_emit(self, eina_playlist_signals[ACTION_ACTIVATED], 0, action, &ret);
	if (ret)
		return;

	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal("remove-action", name))
		playlist_remove_selected(self);
	
	else if (g_str_equal("clear-action", name))
		lomo_player_clear(priv->lomo);

	else if (g_str_equal("queue-action", name))
		playlist_queue_selected(self);

	else if (g_str_equal("dequeue-action", name))
		playlist_dequeue_selected(self);

	else if (g_str_equal("repeat-action", name))
		lomo_player_set_repeat(priv->lomo, gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));

	else if (g_str_equal("random-action", name))
		lomo_player_set_random(priv->lomo, gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));

	else
		g_warning(N_("Unhandled action %s"), name);
}

