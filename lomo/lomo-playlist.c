/*
 * lomo/lomo-playlist.c
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

#include "lomo-playlist.h"

#include <glib/gprintf.h>
#include "lomo.h"

struct _LomoPlaylist {
    GList  *list;
    GList  *random_list;

    gboolean repeat;
    gboolean random;
    gint total;
    gint current;

	gint ref_count;
};

/* * * * * * * * * * */
/* Private functions */
/* * * * * * * * * * */
gint lomo_playlist_normal_to_random
(LomoPlaylist *l, gint pos);
gint lomo_playlist_random_to_normal
(LomoPlaylist *l, gint pos);

/*
 * Return position in the random list
 * of the element at position 'pos' in the normal list 
 */
gint
lomo_playlist_normal_to_random (LomoPlaylist *l, gint pos)
{
	gpointer data = g_list_nth_data(l->list, pos);
	return g_list_index(l->random_list, data);
}

/*
 * Return position in the normal list
 * of the element at position 'pos' in the random list 
  */
gint
lomo_playlist_random_to_normal (LomoPlaylist *l, gint pos)
{
	gpointer data = g_list_nth_data(l->random_list, pos);
	return g_list_index(l->list, data);
}

// --
// Public functions implementation
// --

/**
 * lomo_playlist_new:
 *
 * Creates a new playlist
 *
 * Returns: a new LomoPlaylist
 */
LomoPlaylist*
lomo_playlist_new (void) 
{
	LomoPlaylist *self;

	self = g_new0(LomoPlaylist, 1);
	self->list        = NULL;
	self->random_list = NULL;

	lomo_playlist_set_repeat(self, FALSE);
	lomo_playlist_set_random(self, FALSE);

	self->total       =  0;
	self->current     = -1;
	self->ref_count   = 1;

	return self;
}

/**
 * lomo_playlist_ref:
 * @l: a #LomoPlaylist
 *
 * Adds a reference to the playlist
 */
void
lomo_playlist_ref (LomoPlaylist *l)
{
	l->ref_count++;
}

/*
 * lomo_playlist_unref:
 * @l: a #LomoPlaylist
 *
 * Removes a reference to the playlist
 */
void
lomo_playlist_unref (LomoPlaylist * l)
{
	l->ref_count--;
	if ( l->ref_count > 0 )
		return;

	lomo_playlist_clear(l);
	g_free(l);
}

/**
 * lomo_playlist_get_playlist:
 * @l: a #LomoPlaylist
 *
 * Gets the current elements of the playlist
 *
 * Returns: a #GList of #LomoStream, its owned by liblomo and should not be
 * modified
 */
const GList*
lomo_playlist_get_playlist (LomoPlaylist *l) 
{
	return (const GList *) l->list;
}
/**
 * lomo_playlist_get_random_playlist:
 * @l: a #LomoPlaylist
 *
 * Gets the current elements of the playlist in the internal random order
 *
 * Returns: a newly created #GList of #LomoStream, this should be freeded when
 * no longer needed
 */
GList*
lomo_playlist_get_random_playlist (LomoPlaylist *l) 
{
	return g_list_copy(l->random_list);
}

/*
 * lomo_playlist_get_total:
 * @l: a #LomoPlaylist
 *
 * Gets the number of elements in the playlist
 *
 * Returns: a #gint
 */
gint
lomo_playlist_get_total (LomoPlaylist *l)
{
	return l->total;
}

/*
 * lomo_playlist_get_current:
 * @l: a #LomoPlaylist
 *
 * Gets the index of the current active element or -1 if there is no current
 * element
 *
 * Returns: a #gint
 */
gint
lomo_playlist_get_current (LomoPlaylist *l)
{
	return l->current;
}

/**
 * lomo_playlist_set_current:
 * @l: a #LomoPlaylist
 * @pos: Position index to mark as active
 *
 * Sets the stream on the position pos as active
 *
 * Returns: TRUE if successful, FALSE if index is outside limits
 */
gboolean
lomo_playlist_set_current (LomoPlaylist* l, gint pos)
{
	g_return_val_if_fail(pos <= (lomo_playlist_get_total(l) - 1), FALSE);
	l->current = pos;

	return TRUE;
}

/**
 * lomo_playlist_get_random:
 * @l: a #LomoPlaylist
 *
 * Gets the value for the random flag
 *
 * Returns: a #gboolean representing the value of the flag
 */
gboolean
lomo_playlist_get_random (LomoPlaylist *l)
{
	return l->random;
}

/**
 * lomo_playlist_set_random:
 * @l: a #LomoPlaylist
 * @value: the value for the random flag
 *
 * Sets the value for the random flag
 */
void
lomo_playlist_set_random (LomoPlaylist *l, gboolean val)
{
	l->random = val;
}

/**
 * lomo_playlist_get_repeat:
 * @l: a #LomoPlaylist
 *
 * Gets the value for the repeat flag
 *
 * Returns: a #gboolean representing the value of the flag
 */
gboolean
lomo_playlist_get_repeat (LomoPlaylist *l)
{
	return l->repeat;
}

/**
 * lomo_playlist_set_random:
 * @l: a #LomoPlaylist
 * @value: the value for the random flag
 *
 * Sets the value for the random flag
 */
void
lomo_playlist_set_repeat (LomoPlaylist *l, gboolean val)
{
	l->repeat = val;
}

/**
 * lomo_playlist_get_nth:
 * @l: a #LomoPlaylist
 * @pos: Index to retrieve
 *
 * Gets the #LomoStream at the given position or NULL if the index its outside
 * limits
 *
 * Returns: the #LomoStream
 */
LomoStream*
lomo_playlist_nth_stream(LomoPlaylist *l, guint pos)
{
	g_return_val_if_fail(pos <= (l->total - 1), NULL);
	return g_list_nth_data(l->list, pos);
}

/**
 * lomo_playlist_index:
 * @l: a #LomoPlaylist
 * @stream: the #LomoStream to find
 *
 * Gets the position of the element containing the given data (starting from
 * 0)
 *
 * Returns: the index of the element containing the data, or -1 if the data is
 * not found
 */
gint
lomo_playlist_index(LomoPlaylist *l, LomoStream *stream)
{
	return g_list_index(l->list, stream);
}

/**
 * lomo_playlist_insert:
 * @l: a #LomoPlaylist
 * @stream: the stream to insert
 * @pos: the position to insert the element. If this is negative, or is larger
 * than the number of elements in the list, the new element is added on to the
 * end of the list.
 *
 * Inserts a new element into the list at the given position.
 */
void
lomo_playlist_insert(LomoPlaylist *l, LomoStream *stream, gint pos)
{
	GList *tmp = NULL;

	tmp = g_list_prepend(tmp, stream);
	lomo_playlist_insert_multi(l, tmp, pos);
	g_list_free(tmp);
}

/**
 * lomo_playlist_multi_insert:
 * @l: a #LomoPlaylist
 * @streams: a list of #LomoStreams
 * 
 * @pos: the position to insert the elements. If this is negative, or is larger
 * than the number of elements in the list, the new element is added on to the
 * end of the list.
 *
 * Inserts a list of #LomoStream into the list at the given position.
 */
void lomo_playlist_insert_multi
(LomoPlaylist *l, GList *streams, gint pos)
{
	GList *iter;
	LomoStream *stream;
	guint randompos;

	// pos == -1 means at the end
	if (pos == -1)
		pos = lomo_playlist_get_total(l);
	iter = streams;
	while (iter)
	{
		stream = (LomoStream *) iter->data;

		// Insert into list, each element must be located after previous,
		// so increment pos
		l->list = g_list_insert(l->list, (gpointer) stream, pos++);
		randompos = l->total ? g_random_int_range(0, l->total + 1) : 0;
		l->random_list = g_list_insert(l->random_list,
			(gpointer) stream,
			randompos);

		/* Update total, delay current until while's end */
		l->total++;

		iter = iter->next;
	}

	// Only if it is -1
	if (l->current == -1)
		l->current = 0;
}

/**
 * lomo_playlist_del:
 * @l: a #LomoPlaylist
 * @pos: index of the element to delete
 *
 * Deletes the element at position pos
 */
void lomo_playlist_del
(LomoPlaylist *l, guint pos)
{
	
	LomoStream *stream = (LomoStream *) g_list_nth_data(l->list, pos);
	g_return_if_fail(stream != NULL);

	g_object_unref(stream);

	l->list        = g_list_remove(l->list,        stream);
	l->random_list = g_list_remove(l->random_list, stream);
	l->total--;

	// Check if movement affects current index: 
	// item was the active one or previous to it
	if ( pos <= l->current)
	{
		// Ups, deleted one was the only element in the list, set active to -1
		if (l->total == 0)
			l->current = -1;

		// a bit less complicated, deleted one was the first element, increment
		// active
		else if (pos == 0)
			l->current++;

		// the simpliest case, not 1-element list, and not the firts, decrement
		// active
		else
			l->current--;
	}
}

void lomo_playlist_clear
(LomoPlaylist *l)
{
	if ((l->list != NULL) && (l->list->data != NULL))
	{
		g_list_foreach(l->list, (GFunc) g_object_unref, NULL);

		g_list_free(l->list);
		g_list_free(l->random_list);

		l->list        = NULL;
		l->random_list = NULL;
	}

	l->current = -1;
	l->total = 0;
}

gboolean lomo_playlist_swap
(LomoPlaylist *l, guint a, guint b)
{
	gpointer data_a, data_b;
	gint tmp;
	guint total;

	/* Check if the two elements are inside playlist limits */
	total = lomo_playlist_get_total(l);
	if ( (a < 0) || (a >= total) ||
		 (b < 0) || (b >= total) ||
		 (a == b))
		return FALSE;
	
	/* swap(a,b) if b>a; do this to simplify code */
	if ( b > a ) {
		tmp = a; a = b; b = tmp;
	}

	data_a = g_list_nth_data(l->list, a);
	data_b = g_list_nth_data(l->list, b);

	/* Delete both elements */
	l->list = g_list_remove(l->list, data_a);
	l->list = g_list_remove(l->list, data_b);

	/* Insert 'b' where 'a' was */
	l->list = g_list_insert(l->list, data_b, a - 1);
	l->list = g_list_insert(l->list, data_a, b);

	if      ( a == l->current ) { l->current = b; }
	else if ( b == l->current ) { l->current = a; }

	return TRUE;
}

/*
 * Return position NEXT to the active,
 * or -1 if it doesn't exist
 */
gint lomo_playlist_get_next
(LomoPlaylist *l)
{
	gint pos;
	gint total, normalpos, randompos;

	total     = l->total;
	normalpos = l->current;
	randompos = lomo_playlist_normal_to_random(l, l->current);
	
	// playlist has no elements, return invalid
	if ( total == 0 ) {
		return -1;
	}

	/* At end of list */
	if ( l->random && (randompos == total - 1) ) {
		if ( l->repeat ) { return lomo_playlist_random_to_normal(l, 0);  }
		else             { return -1; }
	}
	
	if ( !(l->random) && (normalpos == total - 1)) {
		if ( l->repeat ) { return 0; }
		else             { return -1; }
	}

	/* normal advance */
	if ( l->random ) {
		pos = lomo_playlist_normal_to_random(l, l->current);
		pos = lomo_playlist_random_to_normal(l, pos + 1);
		return pos;
	}
	else
		return l->current+ 1;
}

/*
 * Return position PREVIOUS to the active,
 * or -1 if it doesn't exist
 */
gint lomo_playlist_get_previous
(LomoPlaylist *l)
{
	gint pos;
	gint total, normalpos, randompos;
	
	total     = l->total;
	normalpos = l->current;
	randompos = lomo_playlist_normal_to_random(l, l->current);

	/* Empty list */
	if ( total == 0 )
		return -1;

	/* At beginning of list */
	if ( l->random && (randompos == 0) ) {
		if ( l->repeat ) { return lomo_playlist_random_to_normal(l, total - 1);  }
		else             { return -1; }
	}
	
	if ( !(l->random) && (normalpos == 0) ) {
		if ( l->repeat ) { return total - 1; }
		else             { return -1; }
	}

	/* normal backwards movement */
	if ( l->random ) {
		pos = lomo_playlist_normal_to_random(l, l->current);
		pos = lomo_playlist_random_to_normal(l, pos - 1);
		return pos;
	}
	else
		return l->current - 1;
}

/*
 * Goto previous element in play list
 * Return TRUE/FALSE on success/failure
 */
gboolean lomo_playlist_go_prev
(LomoPlaylist *l)
{
	gint prev = lomo_playlist_get_previous(l);
	return lomo_playlist_go_nth(l, prev);
}

/* 
 * Goto next element in play list
 * Return TRUE/FALSE on success/failure
 */
gboolean lomo_playlist_go_next
(LomoPlaylist *l)
{
	gint next = lomo_playlist_get_next(l);
	return lomo_playlist_go_nth(l, next);
}

/*
 * Goto element at position 'pos' in play list
 * Return TRUE/FALSE on success/failure
 */
gboolean lomo_playlist_go_nth
(LomoPlaylist *l, gint pos)
{
	g_return_val_if_fail(pos >= 0, FALSE);

	lomo_playlist_set_current(l, pos);

	return TRUE;
}

/* Re-build random list */
void lomo_playlist_randomize
(LomoPlaylist *l)
{
	GList *copy = NULL;
	GList *res  = NULL;
	gpointer data;
	gint i, r, len = lomo_playlist_get_total(l);

	copy = g_list_copy(l->list);

	for (i = 0; i < len; i++) {
		r = g_random_int_range(0, len - i);
		data = g_list_nth_data(copy, r);
		copy = g_list_remove(copy, data);
		res = g_list_prepend(res, data);
	}

	g_list_free(l->random_list);
	l->random_list = res;
}

void lomo_playlist_print
(LomoPlaylist *l)
{
	GList *list;

	list = l->list;
	while (list)
	{
		g_printf("[liblomo] %s\n", (gchar *) g_object_get_data(G_OBJECT(list->data), "uri"));
		list = list->next;
	}
}

void lomo_playlist_print_random
(LomoPlaylist *l)
{
	GList *list;

	list = l->random_list;
	while (list)
	{
		g_printf("[liblomo] %s\n", (gchar *) g_object_get_data(G_OBJECT(list->data), "uri"));
		list = list->next;
	}
}

