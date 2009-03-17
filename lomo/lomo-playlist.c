/*
 * lomo/pl.c
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

#include <glib/gprintf.h>
#include <lomo/lomo-playlist.h>
#include <lomo/lomo-util.h>

#ifdef LOMO_DEBUG
#define BACKTRACE g_printf("[LomoPlaylist Backtrace] %s %d\n", __FUNCTION__, __LINE__);
#else
#define BACKTRACE ((void)(0));
#endif

struct _LomoPlaylist {
    GList *list;
    GList *random_list;

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
{ BACKTRACE
	gpointer data = g_list_nth_data(l->list, pos);
	return g_list_index(l->random_list, data);
}

/*
 * Return position in the normal list
 * of the element at position 'pos' in the random list 
  */
gint
lomo_playlist_random_to_normal (LomoPlaylist *l, gint pos)
{ BACKTRACE
	gpointer data = g_list_nth_data(l->random_list, pos);
	return g_list_index(l->list, data);
}

/* * * * * * * * * * * * * * * * * */
/* Public functions implementation */
/* * * * * * * * * * * * * * * * * */

/* Create new LomoPlaylist object */
LomoPlaylist*
lomo_playlist_new (void) 
{ BACKTRACE
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

void
lomo_playlist_ref (LomoPlaylist *l)
{ BACKTRACE
	l->ref_count++;
}

/* Destroy LomoPlaylist object */
void
lomo_playlist_unref (LomoPlaylist * l)
{ BACKTRACE
	l->ref_count--;
	if ( l->ref_count > 0 )
		return;

#if 0
	// XXX:  Review this, but I think that next comment has some to say about this
	g_list_foreach(l->list, (GFunc) g_object_unref, NULL);

	/* 
	 * Dont do a deep free in playlist.
	 * «We cannot free() what we didn't malloc()»
	 */
	g_list_free(l->list);
	g_list_free(l->random_list);
#endif
	lomo_playlist_clear(l);
	g_free(l);
}

GList*
lomo_playlist_get_playlist (LomoPlaylist *l) 
{ BACKTRACE
	return g_list_copy(l->list);
}

const GList*
lomo_playlist_get_random_playlist (LomoPlaylist *l) 
{ BACKTRACE
	return l->random_list;
}

/* Return number of elements in playlist */
gint
lomo_playlist_get_total (LomoPlaylist *l)
{ BACKTRACE
	return l->total;
}

/* Return position of active element or -1 if no current */
gint
lomo_playlist_get_current (LomoPlaylist *l)
{ BACKTRACE
	return l->current;
}

/* Mark element at position 'pos' as active */
gboolean
lomo_playlist_set_current (LomoPlaylist* l, gint pos)
{ BACKTRACE
	if (pos > (lomo_playlist_get_total(l) - 1))
		return FALSE;

	l->current = pos;

	return TRUE;
}

/* Get random state (TRUE/FALSE) */
gboolean
lomo_playlist_get_random (LomoPlaylist *l)
{ BACKTRACE
	return l->random;
}

/* Set random ON/OFF */
void
lomo_playlist_set_random (LomoPlaylist *l, gboolean val)
{ BACKTRACE
	l->random = val;
}

/* Get repeat state (TRUE/FALSE) */
gboolean
lomo_playlist_get_repeat (LomoPlaylist *l)
{ BACKTRACE
	return l->repeat;
}

/* Set repeat ON/OFF */
void
lomo_playlist_set_repeat (LomoPlaylist *l, gboolean val)
{ BACKTRACE
	l->repeat = val;
}

/* Return element at position 'pos', NULL if off the list */
LomoStream*
lomo_playlist_get_nth(LomoPlaylist *l, guint pos)
{ BACKTRACE
	return g_list_nth_data(l->list, pos);
}

gint
lomo_playlist_get_position(LomoPlaylist *l, LomoStream *stream)
{
	return g_list_index(l->list, stream);
}

void
lomo_playlist_insert(LomoPlaylist *l, LomoStream *stream, gint pos)
{ BACKTRACE
	GList *tmp = NULL;

	tmp = g_list_prepend(tmp, stream);
	lomo_playlist_insert_multi(l, tmp, pos);
	g_list_free(tmp);
}

void lomo_playlist_insert_multi
(LomoPlaylist *l, GList *streams, gint pos)
{ BACKTRACE
	GList *iter;
	LomoStream *stream;
	guint randompos;

	/* Own streams */
	// g_list_foreach(streams, (GFunc) g_object_ref, NULL);

	// pos == -1 means at the end
	if (pos == -1)
		pos = lomo_playlist_get_total(l);

	iter = streams;
	while (iter) {
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

/* Delete element at position 'pos' from playlist */
gint lomo_playlist_del
(LomoPlaylist *l, gint pos)
{ BACKTRACE
	gpointer data;

	// Corner case: not in list
	if ((data = g_list_nth_data(l->list, pos)) == NULL)
		return l->total;

	g_object_unref((LomoStream *) data);
	l->list        = g_list_remove(l->list,        data);
	l->random_list = g_list_remove(l->random_list, data);
	l->total--;

	// Decrement active element number if the deleted was te active one or previous to it
	// XXX: BUG: 
#ifdef OLD_BEHAVIOUR
	if ( pos <= l->current ) {
			l->current--;
	}
#else

	// Check if movement affects current index: 
	// item was the active one or previous to it
	if ( pos <= l->current) {
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
#endif

	return l->total;
}

void lomo_playlist_clear
(LomoPlaylist *l)
{ BACKTRACE
	if ((l->list != NULL) && (l->list->data != NULL)) {
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
{ BACKTRACE
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
{ BACKTRACE
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
gint lomo_playlist_get_prev
(LomoPlaylist *l)
{ BACKTRACE
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
{ BACKTRACE
	gint prev = lomo_playlist_get_prev(l);
	return lomo_playlist_go_nth(l, prev);
}

/* 
 * Goto next element in play list
 * Return TRUE/FALSE on success/failure
 */
gboolean lomo_playlist_go_next
(LomoPlaylist *l)
{ BACKTRACE
	gint next = lomo_playlist_get_next(l);
	return lomo_playlist_go_nth(l, next);
}

/*
 * Goto element at position 'pos' in play list
 * Return TRUE/FALSE on success/failure
 */
gboolean lomo_playlist_go_nth
(LomoPlaylist *l, gint pos)
{ BACKTRACE
	if ( pos < 0 ) {
		return FALSE;
	}
	else {
		lomo_playlist_set_current(l, pos);
		return TRUE;
	}
}

/* Re-build random list */
void lomo_playlist_randomize
(LomoPlaylist *l)
{ BACKTRACE
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
{ BACKTRACE
	GList *list;

	list = l->list;
	while ( list ) {
		g_printf("[liblomo] %s\n", (gchar *) g_object_get_data(G_OBJECT(list->data), "uri"));
		list = list->next;
	}
}

void lomo_playlist_print_random
(LomoPlaylist *l)
{ BACKTRACE
	GList *list;

	list = l->random_list;
	while ( list ) {
		g_printf("[liblomo] %s\n", (gchar *) g_object_get_data(G_OBJECT(list->data), "uri"));
		list = list->next;
	}
}

