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

/**
 * SECTION: eina-playlist
 * @title: EinaPlaylist
 * @short_description: UI component for playlist
 *
 * #EinaPlaylist is the UI component for display and control information
 * relative to the playlist from #LomoPlayer
 */

#include "eina-playlist.h"
#include "eina-playlist-ui.h"
#include <glib/gi18n.h>

#define ENABLE_EXPERIMENTAL 0
#define DEBUG 0
#define DEBUG_PREFIX "EinaPlaylist"
#if DEBUG
#	define debug(...) g_debug(DEBUG_PREFIX " " __VA_ARGS__)
#else
#	define debug(...) ;
#endif

G_DEFINE_TYPE (EinaPlaylist, eina_playlist, GEL_UI_TYPE_GENERIC)

struct _EinaPlaylistPrivate {
	// Props.
	LomoPlayer *lomo;
	gchar *stream_mrkp;

	// Internals
	GtkTreeView  *tv;
	GtkTreeModel *model; // Model, tv has a filter attached
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
guint playlist_signals[LAST_SIGNAL] = { 0 };

enum {
	TAB_PLAYLIST_EMPTY = 0,
	TAB_PLAYLIST_NON_EMPTY = 1
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

void        playlist_set_lomo_player(EinaPlaylist *self, LomoPlayer *lomo);
static void playlist_refresh_model  (EinaPlaylist *self);
static void playlist_insert_stream  (EinaPlaylist *self, LomoStream *stream, gint index);
static void playlist_remove_stream  (EinaPlaylist *self, LomoStream *stream, gint index);
static void playlist_update_state   (EinaPlaylist *self);
static void playlist_change_current (EinaPlaylist *self, gint from, gint to);
static void playlist_update_stream  (EinaPlaylist *self, LomoStream *stream);
static void playlist_remove_selected(EinaPlaylist *self);
static void playlist_queue_selected (EinaPlaylist *self);
static void playlist_queue_stream   (EinaPlaylist *self, LomoStream *stream, gint index, gint queue_index);
static void playlist_dequeue_stream (EinaPlaylist *self, LomoStream *stream, gint index, gint queue_index);

static void     playlist_change_to_activated(EinaPlaylist *self, GtkTreePath *path, GtkTreeViewColumn *column);
static gboolean playlist_react_to_event     (EinaPlaylist *self, GdkEvent *ev);
static void     playlist_handle_action      (EinaPlaylist *self, GtkAction *action);

static gboolean playlist_get_iter_from_index(EinaPlaylist *self, GtkTreeIter *iter, gint index);
static gint*    playlist_get_selected_indices(EinaPlaylist *self);

static gchar* format_stream(LomoStream *stream, gchar *fmt);
static gchar* format_stream_cb(gchar key, LomoStream *stream);

static void
eina_playlist_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	EinaPlaylist *self = EINA_PLAYLIST(object);

	switch (property_id) {
	case PROP_LOMO_PLAYER:
		g_value_set_object(value, eina_playlist_get_lomo_player(self));
		break;
	case PROP_STREAM_MARKUP:
		g_value_set_string(value, eina_playlist_get_stream_markup(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_playlist_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	EinaPlaylist *self = EINA_PLAYLIST(object);

	switch (property_id) {
	case PROP_LOMO_PLAYER:
		playlist_set_lomo_player(self, g_value_get_object(value));
		break;
	case PROP_STREAM_MARKUP:
		eina_playlist_set_stream_markup(self, (gchar *) g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_playlist_dispose (GObject *object)
{
	EinaPlaylist *self = EINA_PLAYLIST(object);
	EinaPlaylistPrivate *priv = self->priv;

	if (priv->lomo)
		playlist_set_lomo_player(self, NULL);
	if (priv->stream_mrkp)
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

	/**
	 * EinaPlaylist:lomo-player:
	 *
	 * The #LomoPlayer associated with this object
	 */
	g_object_class_install_property(object_class, PROP_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "lomo-player", "lomo-player",
			LOMO_TYPE_PLAYER, G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

	/**
	 * EinaPlaylist:stream-markup:
	 *
	 * Markup used to display items in the playlist. The markup will be
	 * surrounded with &lt;b&gt;&lt;/b&gt; if the item is active.
	 *
	 * See SOME-UNDEF-SECTION for more information
	 */
	g_object_class_install_property(object_class, PROP_STREAM_MARKUP,
		g_param_spec_string("stream-markup", "stream-markup", "stream-markup",
			PROP_STREAM_MARKUP_DEFAULT, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

	/**
	 * EinaPlaylist::action-activated:
	 *
	 * Emitted if some action like random, repeat, open, etc is activated and
	 * can be handled by external code
	 *
	 * @playlist: The #EinaPlaylist
	 * @action: The activated #GtkAction
	 *
	 * Returns: %TRUE if action was handled and processing must be stopped,
	 * %FALSE otherwise
	 */
	playlist_signals[ACTION_ACTIVATED] =
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
  	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_PLAYLIST, EinaPlaylistPrivate);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
}

EinaPlaylist*
eina_playlist_new (LomoPlayer *lomo)
{
 	EinaPlaylist *self = g_object_new (EINA_TYPE_PLAYLIST,
		"xml-string", __ui_xml,
		NULL);

 	EinaPlaylistPrivate *priv = self->priv;
	priv->tv    = gel_ui_generic_get_typed(GEL_UI_GENERIC(self), GTK_TREE_VIEW,  "playlist-treeview");
	priv->model = gel_ui_generic_get_typed(GEL_UI_GENERIC(self), GTK_TREE_MODEL, "playlist-model");

	playlist_set_lomo_player(self, lomo);
	g_object_set(
		gel_ui_generic_get_typed(GEL_UI_GENERIC(self), G_OBJECT, "title-renderer"),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_END,
		NULL);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(priv->tv), GTK_SELECTION_MULTIPLE);

	gtk_widget_set_visible(gel_ui_generic_get_typed(self, GTK_WIDGET, "search-frame"), ENABLE_EXPERIMENTAL);

	gtk_activatable_set_related_action(
		gel_ui_generic_get_typed(self, GTK_ACTIVATABLE, "alternative-add-button"),
		gel_ui_generic_get_typed(self, GTK_ACTION, "add-action"));

	return self;
}

/**
 * eina_playlist_get_lomo_player:
 * @self: An #EinaPlaylist
 *
 * Gets the value of #EinaPlaylist:lomo-player property
 *
 * Returns: (transfer none): The property value
 */
LomoPlayer*
eina_playlist_get_lomo_player(EinaPlaylist *self)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), NULL);
	return self->priv->lomo;
}

void
playlist_set_lomo_player(EinaPlaylist *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	self->priv->lomo = g_object_ref(lomo);

	playlist_refresh_model(self);
	g_signal_connect_swapped(lomo, "notify::state", (GCallback) playlist_update_state, self);
	g_signal_connect_swapped(lomo, "insert",   (GCallback) playlist_insert_stream,  self);
	g_signal_connect_swapped(lomo, "remove",   (GCallback) playlist_remove_stream,  self);
	g_signal_connect_swapped(lomo, "clear",    (GCallback) playlist_refresh_model,  self);
	g_signal_connect_swapped(lomo, "change",   (GCallback) playlist_change_current, self);
	g_signal_connect_swapped(lomo, "all-tags", (GCallback) playlist_update_stream,  self);
	g_signal_connect_swapped(lomo, "queue",    (GCallback) playlist_queue_stream,   self);
	g_signal_connect_swapped(lomo, "dequeue",  (GCallback) playlist_dequeue_stream, self);

	// g_signal_connect_swapped(lomo, "queue-clear", (GCallback) playlist_queue_clear,   self);
	g_signal_connect_swapped(self->priv->tv, "row-activated",      (GCallback) playlist_change_to_activated, self);
	g_signal_connect_swapped(self->priv->tv, "key-press-event",    (GCallback) playlist_react_to_event, self);
	g_signal_connect_swapped(self->priv->tv, "button-press-event", (GCallback) playlist_react_to_event, self);

	gchar *actions[] = { "add-action", "remove-action", "clear-action" };
	for (guint i = 0; i < G_N_ELEMENTS(actions); i++)
		g_signal_connect_swapped(gel_ui_generic_get_object(self, actions[i]), "activate", (GCallback) playlist_handle_action, self);

	g_object_bind_property(lomo, "repeat",
		gel_ui_generic_get_typed(self, GTK_TOGGLE_ACTION, "repeat-action"), "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	g_object_bind_property(lomo, "random",
		gel_ui_generic_get_typed(self, GTK_TOGGLE_ACTION, "random-action"), "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

/**
 * eina_playlist_set_stream_markup:
 * @self: An #EinaPlaylist
 * @markup: Value for the property
 *
 * Sets the value of #EinaPlaylist:markup property
 */
void
eina_playlist_set_stream_markup(EinaPlaylist *self, const gchar *markup)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(markup != NULL);

	EinaPlaylistPrivate *priv = self->priv;
	gel_free_and_invalidate(priv->stream_mrkp, NULL, g_free);
	priv->stream_mrkp = markup ? g_strdup(markup) : NULL;

	g_object_notify((GObject *) self, "stream-markup");
}

/**
 * eina_playlist_get_stream_markup:
 * @self: An #EinaPlaylist
 *
 * Gets the value of #EinaPlaylist:markup property
 *
 * Returns: (transfer full): The property value
 */
gchar*
eina_playlist_get_stream_markup(EinaPlaylist *self)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), NULL);
	return g_strdup(self->priv->stream_mrkp);
}

static void
playlist_insert_stream(EinaPlaylist *self, LomoStream *stream, gint index)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = self->priv;

	g_return_if_fail((index >= 0) && (index < lomo_player_get_n_streams(priv->lomo)));

	gchar *text  = format_stream(stream, priv->stream_mrkp);
	gchar *value = g_markup_escape_text(text, -1);

	// If this warning is showed liblomo must be reviewed
	g_warn_if_fail(index != lomo_player_get_current(priv->lomo));

	GtkTreeIter iter;
	gtk_list_store_insert((GtkListStore *) priv->model, &iter, index);
	gtk_list_store_set((GtkListStore *) priv->model, &iter,
		PLAYLIST_COLUMN_MARKUP, value,
		PLAYLIST_COLUMN_TEXT,   value,
		-1);
	g_free(value);

	GtkNotebook *nb = gel_ui_generic_get_typed(self, GTK_NOTEBOOK, "notebook");
	gtk_notebook_set_current_page(nb,
		(lomo_player_get_n_streams(priv->lomo) == 0) ? TAB_PLAYLIST_EMPTY : TAB_PLAYLIST_NON_EMPTY);
}

static void
playlist_remove_stream(EinaPlaylist *self, LomoStream *stream, gint index)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = self->priv;

	// Index must be (0 <= index <= lomo_player_get_n_streams()), remember, stream is already
	// deleted
	g_return_if_fail((index >= 0) && (index <= lomo_player_get_n_streams(priv->lomo)));

	GtkNotebook *nb = gel_ui_generic_get_typed(self, GTK_NOTEBOOK, "notebook");
	gtk_notebook_set_current_page(nb,
		(lomo_player_get_n_streams(priv->lomo) == 0) ? TAB_PLAYLIST_EMPTY : TAB_PLAYLIST_NON_EMPTY);

	GtkTreeIter iter;
	g_return_if_fail(playlist_get_iter_from_index(self, &iter, index));

	gtk_list_store_remove((GtkListStore *) priv->model, &iter);
}

static void
playlist_refresh_model(EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));

	EinaPlaylistPrivate *priv = self->priv;

	GtkNotebook *nb = gel_ui_generic_get_typed(self, GTK_NOTEBOOK, "notebook");
	gtk_notebook_set_current_page(nb,
		lomo_player_get_n_streams(priv->lomo) ? TAB_PLAYLIST_NON_EMPTY : TAB_PLAYLIST_EMPTY);

	gtk_list_store_clear((GtkListStore *) priv->model);

	const GList *streams = lomo_player_get_playlist(priv->lomo);
	GList *l = (GList *) streams;
	gint i = 0;

	while (l)
	{
		playlist_insert_stream(self, (LomoStream *) l->data, i++);
		l = l->next;
	}
}

static void
playlist_update_state(EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));

	EinaPlaylistPrivate *priv = self->priv;

	gint index = lomo_player_get_current(priv->lomo);
	if (index < 0)
		return;
	g_return_if_fail(index < lomo_player_get_n_streams(priv->lomo));

	LomoState state = lomo_player_get_state(priv->lomo);

	GtkTreeIter iter;
	g_return_if_fail(playlist_get_iter_from_index(self, &iter, index));

	gchar *stock = NULL;
	switch (state)
	{
	case LOMO_STATE_STOP:  stock = GTK_STOCK_MEDIA_STOP ; break;
	case LOMO_STATE_PAUSE: stock = GTK_STOCK_MEDIA_PAUSE; break;
	case LOMO_STATE_PLAY:
		stock = ((gtk_widget_get_direction((GtkWidget *) self) == GTK_TEXT_DIR_RTL) ?
			"gtk-media-play-rtl" : "gtk-media-play-ltr");
		break;
	default:
		g_warning(_("Unhanded state: %d"), state);
	}

	gtk_list_store_set((GtkListStore *) priv->model, &iter,
		PLAYLIST_COLUMN_STATE, stock,
		-1);
}

static
void playlist_change_current(EinaPlaylist *self, gint from, gint to)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));

	EinaPlaylistPrivate *priv = self->priv;
	g_return_if_fail(from < lomo_player_get_n_streams(priv->lomo));
	g_return_if_fail(to   < lomo_player_get_n_streams(priv->lomo));

	GtkTreeIter iter;
	gchar *state = NULL;

	if ((from >= 0) && (from != to) && playlist_get_iter_from_index(self, &iter, from))
	{
		gchar *text = NULL;
		gtk_tree_model_get(priv->model, &iter,
			PLAYLIST_COLUMN_TEXT,  &text,
			PLAYLIST_COLUMN_STATE, &state,
			-1);
		gtk_list_store_set((GtkListStore *) priv->model, &iter,
			PLAYLIST_COLUMN_MARKUP, text,
			PLAYLIST_COLUMN_STATE,  NULL,
			-1);
		g_free(text);
	}
	if ((to >= 0) && (from != to) && playlist_get_iter_from_index(self, &iter, to))
	{
		gchar *text = NULL;
		gtk_tree_model_get(priv->model, &iter,
			PLAYLIST_COLUMN_TEXT, &text,
			-1);
		gchar *markup = g_strdup_printf("<b>%s</b>", text);
		gtk_list_store_set((GtkListStore *) priv->model, &iter,
			PLAYLIST_COLUMN_MARKUP, markup,
			PLAYLIST_COLUMN_STATE,  state,
			-1);
		if (state == NULL)
			playlist_update_state(self);
		g_free(markup);
		g_free(text);

		GtkTreePath *tree_path = gtk_tree_path_new_from_indices(to, -1);
		gtk_tree_view_scroll_to_cell(priv->tv, tree_path, NULL, FALSE, 0.0, 0.0);
	}
}

static void
playlist_update_stream (EinaPlaylist *self, LomoStream *stream)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = self->priv;

	gint index = lomo_player_get_stream_index(priv->lomo, stream);
	g_return_if_fail((index >= 0) && (index < lomo_player_get_n_streams(priv->lomo)));

	GtkTreeIter iter;
	g_return_if_fail(playlist_get_iter_from_index(self, &iter, index));

	gchar *tmp  = format_stream(stream, priv->stream_mrkp);
	gchar *text = g_markup_escape_text(tmp, -1);
	g_free(tmp);

	gchar *markup = NULL;

	if (index == lomo_player_get_current(priv->lomo))
		markup = g_strdup_printf("<b>%s</b>", text);

	gtk_list_store_set((GtkListStore *) priv->model, &iter,
		PLAYLIST_COLUMN_TEXT,   text,
		PLAYLIST_COLUMN_MARKUP, markup ? markup : text,
		-1);
	if (markup)
		g_free(markup);
	g_free(text);
}

gint *
playlist_get_selected_indices(EinaPlaylist *self)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), NULL);
	EinaPlaylistPrivate *priv = self->priv;

	GtkTreeSelection *selection = gel_ui_generic_get_typed(self, GTK_TREE_SELECTION, "treeview-selection");

	// Get selected
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &priv->model);
	GList *l = rows;

	// Create an integer list
	guint i = 0;
	gint *ret = g_new0(gint, g_list_length(rows)  + 1);
	while (l)
	{
		GtkTreePath *path = (GtkTreePath *) l->data;
		gint *indices = gtk_tree_path_get_indices(path);

		if (!indices || (indices[0] < 0))
		{
			g_warn_if_fail(!indices || (indices[0] < 0));
			l = l->next;
			continue;
		}

		ret[i++] = indices[0];
		l = l->next;
	}
	ret[i] = -1;

	// Free results
	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	return ret;
}

static void playlist_remove_selected(EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = self->priv;

	gint *indices = playlist_get_selected_indices(self);
	g_return_if_fail(indices && (indices[0] >= 0));

	gint len = 0;
	for (; indices && (indices[len] >= 0); len++);

	for (gint i = (len - 1);  i >= 0; i--)
		lomo_player_remove(priv->lomo, indices[i]);

	g_free(indices);
}

static void
playlist_queue_selected (EinaPlaylist *self)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	EinaPlaylistPrivate *priv = self->priv;

	gint *indices = playlist_get_selected_indices(self);
	g_return_if_fail(indices != NULL);

	for (guint i = 0; indices && (indices[i] != -1); i++)
	{
		LomoStream *stream = lomo_player_get_nth_stream(priv->lomo, indices[i]);
		gint queue_index = lomo_player_queue_get_stream_index(priv->lomo, stream);
		if (queue_index >= 0)
			lomo_player_dequeue(priv->lomo, queue_index);
		else
			lomo_player_queue(priv->lomo, indices[i]);
	}

	g_free(indices);
}

static void
playlist_queue_stream(EinaPlaylist *self, LomoStream *stream, gint index, gint queue_index)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = self->priv;

	g_return_if_fail((index >= 0) && (index < lomo_player_get_n_streams(priv->lomo)));
	g_return_if_fail((queue_index >= 0) && (queue_index < lomo_player_get_n_streams(priv->lomo)));

	GtkTreeIter iter;
	g_return_if_fail(playlist_get_iter_from_index(self, &iter, index));

	gchar *queue_str = g_strdup_printf("<b>%d</b>", queue_index + 1);
	gtk_list_store_set((GtkListStore *) priv->model, &iter,
		PLAYLIST_COLUMN_QUEUE_STR, queue_str,
		-1);
	g_free(queue_str);
}

static void
playlist_dequeue_stream(EinaPlaylist *self, LomoStream *stream, gint index, gint queue_index)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	EinaPlaylistPrivate *priv = self->priv;

	for (guint i = queue_index; i < lomo_player_queue_get_n_streams(priv->lomo); i++)
	{
		gint stream_index = index;
		if (i != queue_index)
		{
			LomoStream *stream = lomo_player_queue_get_nth_stream(priv->lomo, i);
			stream_index = lomo_player_get_stream_index(priv->lomo, stream);
		}

		GtkTreeIter iter;
		if (!playlist_get_iter_from_index(self, &iter, stream_index))
			continue;

		gchar *queue_str = ((i == queue_index) ? NULL : g_strdup_printf("<b>%d</b>", i));
		gtk_list_store_set((GtkListStore *) priv->model, &iter,
			PLAYLIST_COLUMN_QUEUE_STR, queue_str,
			-1);
		if (queue_str)
			g_free(queue_str);
	}
}

static void
playlist_change_to_activated(EinaPlaylist *self,  GtkTreePath *path, GtkTreeViewColumn *column)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(path   != NULL);
	g_return_if_fail(column != NULL);

	EinaPlaylistPrivate *priv = self->priv;

	gint *indices = gtk_tree_path_get_indices(path);
	g_return_if_fail(indices && (indices[0] >= 0));

	gint index = indices[0];
	g_return_if_fail((index >= 0) && (index < lomo_player_get_n_streams(priv->lomo)));

	GError *error = NULL;
	if (!lomo_player_set_current(priv->lomo, index, &error))
	{
		g_warning(_("Unable to set_current to %d: %s"), index, error->message);
		g_error_free(error);
	}

	if (lomo_player_get_state(priv->lomo) != LOMO_STATE_PLAY)
		if (!lomo_player_set_state(priv->lomo, LOMO_STATE_PLAY, &error))
		{
			g_warning(_("Unable to set_sate to LOMO_STATE_PLAY: %s"), error->message);
			g_error_free(error);
		}
}

static
gboolean playlist_react_to_event(EinaPlaylist *self, GdkEvent *event)
{
	GdkEventButton *ev_button = (GdkEventButton *) event;
	if ((event->type == GDK_BUTTON_PRESS) && (ev_button->button == 3))
	{
		GtkMenu *menu = gel_ui_generic_get_typed(self, GTK_MENU, "popup-menu");
		g_return_val_if_fail(menu != NULL, FALSE);

		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, ev_button->button, GDK_CURRENT_TIME);

		return TRUE;
	}

	if (event->type == GDK_KEY_PRESS)
	{
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
	}
	return FALSE;
}

static void
playlist_handle_action(EinaPlaylist *self, GtkAction *action)
{
	g_return_if_fail(EINA_IS_PLAYLIST(self));
	g_return_if_fail(GTK_IS_ACTION(action));

	EinaPlaylistPrivate *priv = self->priv;

	gboolean ret = FALSE;
	g_signal_emit(self, playlist_signals[ACTION_ACTIVATED], 0, action, &ret);
	if (ret)
		return;

        const gchar *name = gtk_action_get_name(action);

	if (g_str_equal("remove-action", name))
		playlist_remove_selected(self);

	else if (g_str_equal("clear-action", name))
		lomo_player_clear(priv->lomo);

	else
		g_warning(_("Unhanded action '%s'"), name);
}

static gboolean
playlist_get_iter_from_index(EinaPlaylist *self, GtkTreeIter *iter, gint index)
{
	g_return_val_if_fail(EINA_IS_PLAYLIST(self), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	EinaPlaylistPrivate *priv = self->priv;

	// Delete restricions on index, sometimes index is out of range, think about
	// ::remove and ::clear signals
	// g_return_val_if_fail((index >= 0) && (index < lomo_player_get_n_streams(priv->lomo)), FALSE);

	GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
	if (!gtk_tree_model_get_iter(priv->model, iter, path))
	{
		gtk_tree_path_free(path);
		g_warning(_("Unable to get GtkTreeIter for index %d"), index);
		return FALSE;
	}
	gtk_tree_path_free(path);
	return TRUE;
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

