#include "lomo-playlist.h"
#include <glib/gprintf.h>

G_DEFINE_TYPE (LomoPlaylist, lomo_playlist, G_TYPE_OBJECT)

struct _LomoPlaylistPrivate {
	GList  *list;
    GList  *random_list;

    gboolean repeat;
    gboolean random;
    gint total;
    gint current;
};

static gint
playlist_normal_to_random (LomoPlaylist *self, gint pos);
static gint
playlist_random_to_normal (LomoPlaylist *self, gint pos);

static void
lomo_playlist_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
lomo_playlist_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
lomo_playlist_dispose (GObject *object)
{
	G_OBJECT_CLASS (lomo_playlist_parent_class)->dispose (object);
}

static void
lomo_playlist_class_init (LomoPlaylistClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoPlaylistPrivate));

	object_class->get_property = lomo_playlist_get_property;
	object_class->set_property = lomo_playlist_set_property;
	object_class->dispose = lomo_playlist_dispose;
}

static void
lomo_playlist_init (LomoPlaylist *self)
{
	LomoPlaylistPrivate *priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), LOMO_TYPE_PLAYLIST, LomoPlaylistPrivate);

	priv->list = priv->random_list = NULL;

	priv->random = priv->repeat = FALSE;

	priv->total   =  0;
	priv->current = -1;
}

/**
 * lomo_playlist_new:
 *
 * Creates a new #LomoPlaylist object
 */
LomoPlaylist*
lomo_playlist_new (void)
{
	return g_object_new (LOMO_TYPE_PLAYLIST, NULL);
}

/**
 * lomo_playlist_get_playlist:
 * @self: a #LomoPlaylist
 *
 * Gets the current elements of the playlist
 *
 * Returns: (transfer none) (element-type Lomo.Stream): List of #LomoStream,
 *          both container and elements are owned by @self
 */
const GList*
lomo_playlist_get_playlist (LomoPlaylist *self) 
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), NULL);
	return (const GList *) self->priv->list;
}

/**
 * lomo_playlist_get_random_playlist:
 * @self: a #LomoPlaylist
 *
 * Gets the current elements of the random playlist
 *
 * Returns: (transfer none) (element-type Lomo.Stream): List of #LomoStream,
 *          both container and elements are owned by @self
 */
const GList*
lomo_playlist_get_random_playlist (LomoPlaylist *self) 
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), NULL);
	return (const GList *) self->priv->random_list;
}

/*
 * lomo_playlist_get_n_streams:
 * @self: a #LomoPlaylist
 *
 * Gets the number of elements in the playlist
 *
 * Returns: The number of strems
 */
gint
lomo_playlist_get_n_streams(LomoPlaylist *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	return self->priv->total;
}

/*
 * lomo_playlist_get_current:
 * @self: a #LomoPlaylist
 *
 * Gets the index of the current active element or -1 if there is no current
 * element
 *
 * Returns: The index of current stream
 */
gint
lomo_playlist_get_current (LomoPlaylist *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	return self->priv->current;
}

/**
 * lomo_playlist_set_current:
 * @self: a #LomoPlaylist
 * @index: Position index to mark as active
 *
 * Sets the stream on the position pos as active
 *
 * Returns: TRUE if successful, FALSE if index is outside limits
 */
gboolean
lomo_playlist_set_current (LomoPlaylist *self, gint index)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), FALSE);
	g_return_val_if_fail(index < lomo_playlist_get_n_streams(self), FALSE);

	self->priv->current = index;

	return TRUE;
}

/**
 * lomo_playlist_get_random:
 * @self: a #LomoPlaylist
 *
 * Gets the value for the random flag
 *
 * Returns: The random value
 */
gboolean
lomo_playlist_get_random (LomoPlaylist *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), FALSE);
	return self->priv->random;
}

/**
 * lomo_playlist_set_random:
 * @self: a #LomoPlaylist
 * @random: the value for the random flag
 *
 * Sets the value for the random flag. If @value is TRUE, playlist is
 * randomized
 */
void
lomo_playlist_set_random (LomoPlaylist *self, gboolean random)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));
	
	if (random == self->priv->random)
		return;

	self->priv->random = random;
	if (random)
		lomo_playlist_randomize(self);
}

/**
 * lomo_playlist_get_repeat:
 * @self: a #LomoPlaylist
 *
 * Gets the value for the repeat flag
 *
 * Returns: The repeat value
 */
gboolean
lomo_playlist_get_repeat (LomoPlaylist *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), FALSE);
	return self->priv->repeat;
}

/**
 * lomo_playlist_set_random:
 * @self: a #LomoPlaylist
 * @value: the value for the random flag
 *
 * Sets the value for the random flag
 */
void
lomo_playlist_set_repeat (LomoPlaylist *self, gboolean repeat)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));
	self->priv->repeat = repeat;
}

/**
 * lomo_playlist_get_nth_stream
 * @self: a #LomoPlaylist
 * @index: Index to retrieve
 *
 * Gets the #LomoStream at the given position or NULL if the index its outside
 * limits
 *
 * Returns: (transfer none): the #LomoStream or %NULL
 */
LomoStream*
lomo_playlist_get_nth_stream(LomoPlaylist *self, gint index)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), NULL);
	g_return_val_if_fail((index >= 0) && (index < lomo_playlist_get_n_streams(self)), NULL);

	return LOMO_STREAM(g_list_nth_data(self->priv->list, index));
}

/**
 * lomo_playlist_get_stream_index:
 * @self: a #LomoPlaylist
 * @stream: (transfer none): the #LomoStream to find
 *
 * Gets the position of the element containing the given data (starting from
 * 0)
 *
 * Returns: the index of the element containing the data, or -1 if the stream is
 * not found
 */
gint
lomo_playlist_get_stream_index(LomoPlaylist *self, LomoStream *stream)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), -1);

	return g_list_index(self->priv->list, stream);
}

/**
 * lomo_playlist_insert:
 * @self: a #LomoPlaylist
 * @stream: (transfer none): the stream to insert
 * @index: the position to insert the element. If this is negative, or is larger
 * than the number of elements in the list, the new element is added on to the
 * end of the list.
 *
 * Inserts a new element into the list at the given position.
 */
void
lomo_playlist_insert(LomoPlaylist *self, LomoStream *stream, gint index)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));

	GList *tmp = g_list_prepend(NULL, stream);
	lomo_playlist_insert_multi(self, tmp, index);
	g_list_free(tmp);
}

/**
 * lomo_playlist_multi_insert:
 * @self: a #LomoPlaylist
 * @streams: (transfer none) (element-type Lomo.Stream): a list of #LomoStreams
 * @index: the position to insert the elements. If this is negative, or is larger
 *         than the number of elements in the list, the new element is added on to the
 *         end of the list.
 *
 * Inserts a list of #LomoStream into the list at the given position.
 */
void
lomo_playlist_insert_multi (LomoPlaylist *self, GList *streams, gint index)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));

	LomoPlaylistPrivate *priv = self->priv;
	
	if ((index < 0) || (index > lomo_playlist_get_n_streams(self)))
		index = lomo_playlist_get_n_streams(self);

	GList *iter = streams;
	while (iter)
	{
		LomoStream *stream = (LomoStream *) iter->data;
		if (!LOMO_IS_STREAM(stream))
		{
			g_warn_if_fail(LOMO_IS_STREAM(stream));
			iter = iter->next;
			continue;
		}

		// Insert into list, each element must be located after previous,
		// so increment pos
		priv->list     = g_list_insert(priv->list, (gpointer) g_object_ref(stream), index++);
		gint randompos = priv->total ? g_random_int_range(0, priv->total + 1) : 0;

		priv->random_list = g_list_insert(priv->random_list, (gpointer) stream, randompos);

		priv->total++;
		iter = iter->next;
	}

	// Important: Dont move 'current'.
}

/**
 * lomo_playlist_remove:
 * @self: a #LomoPlaylist
 * @index: index of the element to delete
 *
 * Deletes the element at position @index
 */
void
lomo_playlist_remove (LomoPlaylist *self, gint index)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));
	g_return_if_fail((index >= 0) && (index < lomo_playlist_get_n_streams(self)));

	LomoPlaylistPrivate *priv = self->priv;

	LomoStream *stream = (LomoStream *) g_list_nth_data(priv->list, index);
	g_return_if_fail(LOMO_IS_STREAM(stream));

	priv->list        = g_list_remove(priv->list,        stream);
	priv->random_list = g_list_remove(priv->random_list, stream);
	priv->total--;
	g_object_unref(stream);

	// Check if movement affects current index: 
	// item was the active one or previous to it
	if (index <= priv->current)
	{
		// Ups, deleted one was the only element in the list, set active to -1
		if (lomo_playlist_get_n_streams(self) == 0)
			lomo_playlist_set_current(self, -1);

		// a bit less complicated, deleted one was the first element, increment
		// active
		else if (index == 0)
			lomo_playlist_set_current(self, lomo_playlist_get_current(self) + 1);

		// the simpliest case, not 1-element list, and not the firts, decrement
		// active
		else
			lomo_playlist_set_current(self, lomo_playlist_get_current(self) - 1);
	}
}

/**
 * lomo_playlist_clear:
 * @self: A #LomoPlaylist
 *
 * Clears the playlist
 */
void
lomo_playlist_clear(LomoPlaylist *self)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));
	LomoPlaylistPrivate *priv = self->priv;

	
	g_list_foreach(priv->list, (GFunc) g_object_unref, NULL);
	g_list_free(priv->list);
	g_list_free(priv->random_list);

	priv->list        = NULL;
	priv->random_list = NULL;

	lomo_playlist_set_current(self, -1);
	priv->total = 0;
}

/**
 * lomo_playlist_swap:
 * @self: A #LomoPlaylist
 * @a: First index
 * @b: Second index
 *
 * Swap position of two streams in the playlist
 *
 * Returns: %TRUE if successful, %FALSE otherwise
 */
gboolean
lomo_playlist_swap (LomoPlaylist *self, gint a, gint b)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), FALSE);
	LomoPlaylistPrivate *priv = self->priv;

	/* Check if the two elements are inside playlist limits */
	gint total = lomo_playlist_get_n_streams(self);
	g_return_val_if_fail(
	    (a < 0) || (a >= total) ||
		(b < 0) || (b >= total) ||
		(a == b), FALSE);
	
	/* swap(a,b) if b>a; do this to simplify code */
	if ( b > a )
	{
		gint tmp = a;
		a = b;
		b = tmp;
	}

	gpointer data_a = g_list_nth_data(priv->list, a);
	gpointer data_b = g_list_nth_data(priv->list, b);

	/* Delete both elements */
	priv->list = g_list_remove(priv->list, data_a);
	priv->list = g_list_remove(priv->list, data_b);

	/* Insert 'b' where 'a' was */
	priv->list = g_list_insert(priv->list, data_b, a - 1);
	priv->list = g_list_insert(priv->list, data_a, b);

	gint new_curr = -1;
	if ( a == priv->current )
		new_curr = b;
	else if ( b == priv->current )
		new_curr = a;

	if (new_curr >= 0)
		lomo_playlist_set_current(self, new_curr);

	return TRUE;
}

/**
 * lomo_playlist_get_next:
 * @self: A #LomoPlaylist
 *
 * Return the index next to current
 *
 * Returns: The next index
 */
gint
lomo_playlist_get_next (LomoPlaylist *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	LomoPlaylistPrivate *priv = self->priv;

	gint total     = lomo_playlist_get_n_streams(self);
	gint normalpos = lomo_playlist_get_current(self);
	gint randompos = playlist_normal_to_random(self, normalpos);
	
	// playlist has no elements, return invalid
	if (total == 0)
		return -1;

	/* At end of list */
	if (priv->random && (randompos == total - 1))
		return priv->repeat ? playlist_random_to_normal(self, 0) : -1;

	if (!(priv->random) && (normalpos == total - 1))
		return priv->repeat ? 0 : -1;

	/* normal advance */
	if ( priv->random )
		return playlist_random_to_normal(self, playlist_normal_to_random(self, normalpos)  + 1);
	else
		return normalpos + 1;
}

/**
 * lomo_playlist_get_previous:
 * @self: A #LomoPlaylist
 *
 * Return the index previous to current
 *
 * Returns: The previous index
 */
gint
lomo_playlist_get_previous (LomoPlaylist *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	LomoPlaylistPrivate *priv = self->priv;

	gint total     = lomo_playlist_get_n_streams(self);
	gint normalpos = lomo_playlist_get_current(self);
	gint randompos = playlist_normal_to_random(self, normalpos);

	/* Empty list */
	if (total == 0)
		return -1;

	/* At beginning of list */
	if (priv->random && (randompos == 0))
		return priv->repeat ? playlist_random_to_normal(self, total - 1) : -1;

	if (!(priv->random) && (normalpos == 0))
		return priv->repeat ? total - 1 : -1;

	/* normal backwards movement */
	if ( priv->random )
		return playlist_random_to_normal(self, playlist_normal_to_random(self, normalpos) - 1);
	else
		return normalpos - 1;
}

/**
 * lomo_playlist_randomize:
 * @self: A #LomoPlaylist
 *
 * Randomized playlist
 */
void
lomo_playlist_randomize (LomoPlaylist *self)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));
	LomoPlaylistPrivate *priv = self->priv;

	GList *copy = NULL;
	GList *res  = NULL;
	gpointer data;
	gint i, r, len = lomo_playlist_get_n_streams(self);

	copy = g_list_copy((GList *) lomo_playlist_get_playlist(self));

	for (i = 0; i < len; i++)
	{
		r = g_random_int_range(0, len - i);
		data = g_list_nth_data(copy, r);
		copy = g_list_remove(copy, data);
		res = g_list_prepend(res, data);
	}

	g_list_free(priv->random_list);
	priv->random_list = res;
}

/**
 * lomo_playlist_print:
 * self: a #LomoPlaylist
 *
 * Print the internal playlist
 */
void
lomo_playlist_print (LomoPlaylist *self)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));

 	GList *list = (GList *) lomo_playlist_get_playlist(self);
	while (list)
	{
		g_printf("[liblomo] %s\n", (gchar *) g_object_get_data(G_OBJECT(list->data), "uri"));
		list = list->next;
	}
}

/**
 * lomo_playlist_print_random:
 * @self: A #LomoPlaylist
 *
 * Prints the random playlist
 */
void
lomo_playlist_print_random(LomoPlaylist *self)
{
	g_return_if_fail(LOMO_IS_PLAYLIST(self));

	GList *list = (GList *) lomo_playlist_get_random_playlist(self);
	while (list)
	{
		g_printf("[liblomo] %s\n", (gchar *) g_object_get_data(G_OBJECT(list->data), "uri"));
		list = list->next;
	}
}

/**
 * playlist_normal_to_random:
 * @self: A #LomoPlaylist
 * @pos: Index in normal playlist
 *
 * Transform normal index @pos to random index
 *
 * Returns: The index in the random mode
 */
static gint
playlist_normal_to_random (LomoPlaylist *self, gint pos)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	LomoPlaylistPrivate *priv = self->priv;

	gpointer data = g_list_nth_data(priv->list, pos);
	return g_list_index(priv->random_list, data);
}

/**
 * playlist_random_to_normal:
 * @self: A #LomoPlaylist
 * @pos: Index in random playlist
 *
 * Transform random index @pos to normal index
 *
 * Returns: The index in the normal mode
 */
static gint
playlist_random_to_normal (LomoPlaylist *self, gint pos)
{
	g_return_val_if_fail(LOMO_IS_PLAYLIST(self), -1);
	LomoPlaylistPrivate *priv = self->priv;

	gpointer data = g_list_nth_data(priv->random_list, pos);
	return g_list_index(priv->list, data);
}


