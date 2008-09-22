#ifndef __LIBLOMO_PLAYLIST_H__
#define __LIBLOMO_PLAYLIST_H__

#include <glib.h>
#include "stream.h"

G_BEGIN_DECLS

typedef struct _LomoPlaylist LomoPlaylist;

/* 
 * @lomo_playlist_new
 * @ Creates a new playlist
 *  LomoPlaylist* ret: The new LomoPlaylist pseudo-object
 */
LomoPlaylist *lomo_playlist_new(void);

void lomo_playlist_ref(LomoPlaylist *l);

/*
 * @lomo_playlist_unref
 * @ Frees all resources from a LomoPlaylist object
 *  LomoPlaylist* l: [self]
 */
void lomo_playlist_unref(LomoPlaylist *l);

/* 
 * @lomo_playlist_add
 * @ Adds a LomoStream to playlist
 *  LomoPlaylist* l: [self]
 *  LomoStream* stream: Stream to add, it cannot be freed without previous lomo_playlist_del call
 *  gint ret: Position of the stream on playlist
 */
#define lomo_playlist_add(l,s) lomo_playlist_add_at_pos(l,s,-1)
gint lomo_playlist_add_at_pos(LomoPlaylist *l, LomoStream *stream, gint pos);

/*
 * @lomo_playlist_add_multi
 * @ Adds multiple streams into playlist at once
 *  LomoPlaylist* l: [self]
 *  GList* streams: List of LomoStream's to add
 *  gint ret: Position of the first stream on playlist
 */
#define lomo_playlist_add_multi(l,list) lomo_playlist_add_multi_at_pos(l,list,-1)
gint lomo_playlist_add_multi_at_pos(LomoPlaylist *l, GList *streams, gint pos);

/*
 * @lomo_player_del
 * @ Deletes the LomoStream at position 'pos' from playlist
 *  LomoPlaylist* l: [self]
 *  gint ret: Items in the playlist
 */
gint lomo_playlist_del(LomoPlaylist * l, gint pos);

/*
 * @lomo_playlist_clear
 * @ Deletes all streams from playlist, this is not a deep free, LomoStream
 * @ contained in playlist sould be freed manualy
 *  LomoPlaylist* l: [self]
 */
void lomo_playlist_clear(LomoPlaylist *l);

/*
 * @lomo_playlist_get_playlist
 * @ Returns a GList of LomoStreams contained in playlist
 *  LomoPlaylist* l: [self]
 *  GList* ret: A GList of LomoStreams
 */
const GList *lomo_playlist_get_playlist(LomoPlaylist *l);

/*
 * @lomo_playlist_get_random_playlist
 * @ Returns a GList of LomoStreams contained in random playlist
 *  LomoPlaylist* l: [self]
 *  GList* ret: A GList of LomoStreams
 */
const GList* lomo_playlist_get_random_playlist (LomoPlaylist *l);

/*
 * @lomo_playlist_get_nth
 * @Returns the LomoStream at position 'pos'
 *  LomoPlaylist* l: [self]
 *  guint pos: Postion to query
 *  const LomoStream* ret: LomoStream located at position 'pos'
 */
const LomoStream *lomo_playlist_get_nth(LomoPlaylist *l, guint pos);

/*
 * @lomo_playlist_total
 * @Gets the number of streams currently in playlist
 *  LomoPlaylist* l: [self]
 *  gint ret: The number of streams in playlist
 */
gint lomo_playlist_get_total (LomoPlaylist *l);

/*
 * @lomo_playlist_get_current
 * @Gets active stream in playlist, returns -1 if thereis no active element
 *  LomoPlaylist* l: [self]
 *  gint ret: Active stream
 */
gint lomo_playlist_get_current(LomoPlaylist *l);

/* @lomo_playlist_set_current
 * @Sets active stream in playlist. -1 sets no active element
 *  LomoPlaylist* l: [self]
 *  gint pos: Number of stream to set as active
 *  gboolean ret: Operation is ok
 */
gboolean lomo_playlist_set_current(LomoPlaylist *l, gint pos);

/* 
 * @lomo_playlist_get_random
 * @Returns if random mode is active or not
 *  LomoPlaylist* l: [self]
 *  gboolean ret: TRUE is random is active, otherwise returns FALSE
 */ 
gboolean lomo_playlist_get_random(LomoPlaylist *l);

/* 
 * @lomo_playlist_set_random
 * @Sets or unsets random mode
 *  LomoPlaylist* l: [self]
 *  gboolean val: TRUE for set random on, FALSE for off
 */
void lomo_playlist_set_random(LomoPlaylist *l, gboolean val);

/* 
 * @lomo_playlist_get_repeat
 * @Returns if repeat mode is active or not
 *  LomoPlaylist* l: [self]
 *  gboolean ret: TRUE is repeat is active, otherwise returns FALSE
 */ 
gboolean lomo_playlist_get_repeat (LomoPlaylist *l);

/* 
 * @lomo_playlist_set_repeat
 * @Sets or unsets repeat mode
 *  LomoPlaylist* l: [self]
 *  gboolean val: TRUE for set repeat on, FALSE for off
 */
void lomo_playlist_set_repeat(LomoPlaylist *l, gboolean val);

/*
 * @lomo_playlist_get_prev
 * @Returns the previous postion to active stream, if there is no previous position
 * @retuns -1
 *  LomoPlaylist* l: [self]
 *  gint ret: Previous position to active stream
 */
gint lomo_playlist_get_prev(LomoPlaylist *l);

/*
 * @lomo_playlist_get_next
 * @Returns the next postion to active stream, if there is no next position
 * @retuns -1
 *  LomoPlaylist* l: [self]
 *  gint ret: Next position to active stream
 */
gint lomo_playlist_get_next(LomoPlaylist *l);

/*
 * @lomo_playlist_go_prev
 * @Changes active stream to the previous available one. If there is previous stream, returns
 * @FALSE, otherwise returns FALSE.
 * @This is equivalent to do:
 * @ i = lomo_playlist_get_prev(l);
 * @ lomo_playlist_set_current(l, i);
 *  LomoPlaylist* l: [self]
 *  gboolean ret: If LomoPlaylist was able to change to previous stream
 */
gboolean lomo_playlist_go_prev (LomoPlaylist *l);

/*
 * @lomo_playlist_go_next
 * @Changes active stream to the next available one. If there is next stream, returns
 * @FALSE, otherwise returns FALSE.
 * @This is equivalent to do:
 * @ i = lomo_playlist_get_next(l);
 * @ lomo_playlist_set_current(l, i);
 *  LomoPlaylist* l: [self]
 *  gboolean ret: If LomoPlaylist was able to change to next stream
 */
gboolean lomo_playlist_go_next (LomoPlaylist *l);

/*
 * @lomo_playlist_go_nth
 * @Changes active stream to the stream 'pos'.
 * @If pos is equal to -1, active is set to -1
 *  LomoPlaylist* l: [self]
 *  gboolean ret: If LomoPlaylist was able to change to stream 'pos'. If pos is equal to -1, returns TRUE
 */
gboolean lomo_playlist_go_nth (LomoPlaylist *l, gint pos);

/*
 * @lomo_playlist_swap
 * @Swaps streams at positions 'a' and 'b', returns TRUE on successful operation, FALSE otherwise
 *  LomoPlaylist* l: [self]
 *  guint a: First element to swap
 *  guint b: Second element to swap
 *  gboolean ret: Successful status of operation
 */
gboolean lomo_playlist_swap(LomoPlaylist *l, guint a, guint b);

/* 
 * @lomo_playlist_randomize
 * @Rebuilds internal random list of playlist
 *  LomoPlaylist* l: [self]
 */
void lomo_playlist_randomize(LomoPlaylist *l);

#if 0
/* 
 * @lomo_playlist_find
 * @Finds the position in playlist of one stream
 * @Returns -1 if the stream is not found inside playlist
 *  LomoPlaylist* l: [self]
 *  LomoStream* stream: LomoStream to find
 *  gint ret: Position of the stream in playlist or -1 if not found
 */
gint lomo_playlist_find(LomoPlaylist *l, LomoStream *stream);
#endif

/*
 * @lomo_playlist_print
 * @Prints to stdout the internal playlist
 *  LomoPlaylist* l: [self]
 */
void lomo_playlist_print(LomoPlaylist *l);

/*
 * @lomo_playlist_print_random
 * @Prints to stdout the internal playlist in random mode
 *  LomoPlaylist* l: [self]
 */
void lomo_playlist_print_random(LomoPlaylist *l);

G_END_DECLS

#endif /* __LIBLOMO_PLAYLIST_H__ */
