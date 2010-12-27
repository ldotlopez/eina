/*
 * lomo/lomo-player.h
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

#ifndef _LOMO_PLAYER
#define _LOMO_PLAYER

#include <glib-object.h>
#include <gst/gst.h>
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define LOMO_TYPE_PLAYER lomo_player_get_type()

#define LOMO_PLAYER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_PLAYER, LomoPlayer))

#define LOMO_PLAYER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), LOMO_TYPE_PLAYER, LomoPlayerClass))

#define LOMO_IS_PLAYER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_PLAYER))

#define LOMO_IS_PLAYER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOMO_TYPE_PLAYER))

#define LOMO_PLAYER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), LOMO_TYPE_PLAYER, LomoPlayerClass))

typedef struct {
	GObject parent;
} LomoPlayer;

typedef struct {
	/*< private >*/
	GObjectClass parent_class;

	void (*play)        (LomoPlayer *self);
	void (*pause)       (LomoPlayer *self);
	void (*stop)        (LomoPlayer *self);
	void (*seek)        (LomoPlayer *self, gint64 old, gint64 new);
	void (*volume)      (LomoPlayer *self, gint volume);
	void (*mute)        (LomoPlayer *self, gboolean mute);
	void (*insert)      (LomoPlayer *self, LomoStream *stream, gint pos);
	void (*remove)      (LomoPlayer *self, LomoStream *stream, gint pos);
	void (*queue)       (LomoPlayer *self, LomoStream *stream, gint pos);
	void (*dequeue)     (LomoPlayer *self, LomoStream *stream, gint pos);
	void (*queue_clear) (LomoPlayer *self);
	void (*pre_change)  (LomoPlayer *self);
	void (*change)      (LomoPlayer *self, gint from, gint to);
	void (*clear)       (LomoPlayer *self);
	void (*repeat)      (LomoPlayer *self, gboolean val);
	void (*random)      (LomoPlayer *self, gboolean val);
	void (*eos)         (LomoPlayer *self);
	void (*error)       (LomoPlayer *self, GError *error);
	void (*tag)         (LomoPlayer *self, LomoStream *stream, LomoTag tag);
	void (*all_tags)    (LomoPlayer *self, LomoStream *stream);
} LomoPlayerClass;

typedef struct _LomoPlayerVTable {
	GstElement* (*create_pipeline)  (const gchar *uri, GHashTable *opts);
	void        (*destroy_pipeline) (GstElement *pipeline);

	GstStateChangeReturn (*set_state) (GstElement *pipeline, GstState state);
	GstState             (*get_state) (GstElement *pipeline);

	gboolean (*set_position) (GstElement *pipeline, GstFormat  format, gint64  position);
	gboolean (*get_position) (GstElement *pipeline, GstFormat *format, gint64 *position);
	gboolean (*get_length)   (GstElement *pipeline, GstFormat *format, gint64 *duration);

	// 0 lowest, 100 highest (there is not a common range over all posible
	// sinks, so make it relative and let vfunc set it
	gboolean (*set_volume) (GstElement *pipeline, gint volume);
	gint     (*get_volume) (GstElement *pipeline);

	// Can be omited
	gboolean (*set_mute) (GstElement *pipeline, gboolean mute);
	gboolean (*get_mute) (GstElement *pipeline);
} LomoPlayerVTable;

/* LomoPlayer errors */
enum {
	LOMO_PLAYER_NO_ERROR = 0,
	LOMO_PLAYER_ERROR_MISSING_METHOD,
	LOMO_PLAYER_ERROR_CREATE_PIPELINE,
	LOMO_PLAYER_ERROR_MISSING_PIPELINE,
	LOMO_PLAYER_ERROR_SET_STATE,
	LOMO_PLAYER_ERROR_CANNOT_DEQUEUE,
	LOMO_PLAYER_ERROR_UNKNOW_STATE,
	LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE,
	LOMO_PLAYER_ERROR_NO_STREAM,
	LOMO_PLAYER_HOOK_BLOCK
};

/**
 * LomoPlayerHookType:
 * @LOMO_PLAYER_HOOK_PLAY: Play hook
 * @LOMO_PLAYER_HOOK_PAUSE: Pause hook
 * @LOMO_PLAYER_HOOK_STOP: Stop hook
 * @LOMO_PLAYER_HOOK_SEEK: Seek hook
 * @LOMO_PLAYER_HOOK_VOLUME: Volume hook
 * @LOMO_PLAYER_HOOK_MUTE: Mute hook
 * @LOMO_PLAYER_HOOK_INSERT: Insert hook
 * @LOMO_PLAYER_HOOK_REMOVE : Remove hook
 * @LOMO_PLAYER_HOOK_QUEUE: Queue hook
 * @LOMO_PLAYER_HOOK_DEQUEUE: Dequeue hook
 * @LOMO_PLAYER_HOOK_QUEUE_CLEAR: Queue clear hook
 * @LOMO_PLAYER_HOOK_CHANGE: Changed hook
 * @LOMO_PLAYER_HOOK_CLEAR: Clear hook
 * @LOMO_PLAYER_HOOK_REPEAT: Repeat hook
 * @LOMO_PLAYER_HOOK_RANDOM: Random hook
 * @LOMO_PLAYER_HOOK_EOS: EOS hook
 * @LOMO_PLAYER_HOOK_ERROR: Error hook
 * @LOMO_PLAYER_HOOK_TAG: Tag hook
 * @LOMO_PLAYER_HOOK_ALL_TAGS: All tags hook
 *
 * Determines the type of hook
 **/
typedef enum {
	LOMO_PLAYER_HOOK_PLAY,
	LOMO_PLAYER_HOOK_PAUSE,
	LOMO_PLAYER_HOOK_STOP,
	LOMO_PLAYER_HOOK_SEEK,
	LOMO_PLAYER_HOOK_VOLUME,
	LOMO_PLAYER_HOOK_MUTE,
	LOMO_PLAYER_HOOK_INSERT,
	LOMO_PLAYER_HOOK_REMOVE,
	LOMO_PLAYER_HOOK_QUEUE,
	LOMO_PLAYER_HOOK_DEQUEUE,
	LOMO_PLAYER_HOOK_QUEUE_CLEAR,
	LOMO_PLAYER_HOOK_CHANGE,
	LOMO_PLAYER_HOOK_CLEAR,
	LOMO_PLAYER_HOOK_REPEAT,
	LOMO_PLAYER_HOOK_RANDOM,
	LOMO_PLAYER_HOOK_EOS,
	LOMO_PLAYER_HOOK_ERROR,
	LOMO_PLAYER_HOOK_TAG,
	LOMO_PLAYER_HOOK_ALL_TAGS
} LomoPlayerHookType;

/**
 * LomoPlayerHookEvent:
 * @type: Type of the event
 * @old: Old position (seek type event)
 * @new: New position (seek type event)
 * @volume: Volumen value (volume type event)
 * @stream: Stream object (insert, remove, queue, dequeue, tag and all_tags
 *          event types)
 * @pos: Position (insert and remove event types)
 * @queue_pos: Queue position (queue and dequeue event types)
 * @from: From position (change event type)
 * @to: From position (change event type)
 * @tag: Tag value (tag event type)
 * @value: %TRUE or %FALSE (randonm, repeat and mute event types)
 * @error: A #GError (error event type)
 *
 * Packs relative for a concrete event, depending the type of the event more or
 * less fields will be set, others will be unconcrete.
 **/
typedef struct {
	LomoPlayerHookType type;
	gint old, new;      // seek
	gint volume;        // volume
	LomoStream *stream; // insert, remove, queue, dequeue, tag, all_tags
	gint pos;           // insert, remove
	gint queue_pos;     // queue, dequeue
	gint from, to;      // change
	LomoTag tag;        // tag
	gboolean value;     // random, repeat, mute
	GError *error;      // error
} LomoPlayerHookEvent;

typedef gboolean(*LomoPlayerHook)(LomoPlayer *self, LomoPlayerHookEvent event, gpointer ret, gpointer data);

/**
 * LomoStateChangeReturn:
 * @LOMO_STATE_CHANGE_SUCCESS: The state has changed
 * @LOMO_STATE_CHANGE_ASYNC: State change will append async
 * @LOMO_STATE_CHANGE_NO_PREROLL: See %GST_STATE_CHANGE_NO_PREROLL
 * @LOMO_STATE_CHANGE_FAILURE: State change has failed.
 *
 * Defines how the state change is performed after a lomo_player_set_state()
 * call
 **/
typedef enum {
	LOMO_STATE_CHANGE_SUCCESS     = GST_STATE_CHANGE_SUCCESS,
	LOMO_STATE_CHANGE_ASYNC       = GST_STATE_CHANGE_ASYNC,
	LOMO_STATE_CHANGE_NO_PREROLL  = GST_STATE_CHANGE_NO_PREROLL,
	LOMO_STATE_CHANGE_FAILURE     = GST_STATE_CHANGE_FAILURE,
} LomoStateChangeReturn;

/**
 * LomoState
 * @LOMO_STATE_INVALID: Invalid state
 * @LOMO_STATE_STOP: Stop state
 * @LOMO_STATE_PLAY: Play state
 * @LOMO_STATE_PAUSE: Pause state
 * @LOMO_N_STATES: Helper define
 *
 * Defines the state of the #LomoPlayer object
 **/
typedef enum {
	LOMO_STATE_INVALID = -1,
	LOMO_STATE_STOP    = 0,
	LOMO_STATE_PLAY    = 1,
	LOMO_STATE_PAUSE   = 2,

	LOMO_N_STATES
} LomoState;

/**
 * LomoFormat:
 * @LOMO_FORMAT_INVALID: Invalid format
 * @LOMO_FORMAT_TIME: Format is time
 * @LOMO_FORMAT_PERCENT: Format is precent
 * @LOMO_N_FORMATS: Helper define
 *
 * Define in which format data is expressed
 **/
typedef enum {
	LOMO_FORMAT_INVALID = -1,
	LOMO_FORMAT_TIME    = 0,
	LOMO_FORMAT_PERCENT = 1,

	LOMO_N_FORMATS
} LomoFormat;

GType lomo_player_get_type (void);

/**
 * lomo_init:
 * @argc: argc from main()
 * @argv: argv from main()
 *
 * Initializes liblomo
 */
#define lomo_init(argc,argv)    gst_init(argc,argv)

/**
 * lomo_get_option_group:
 *
 * Gets the default option group for liblomo
 *
 * Returns: a #GOptionGroup
 */
#define lomo_get_option_group() gst_init_get_option_group()

LomoPlayer* lomo_player_new (gchar *option_name, ...);

gboolean lomo_player_get_auto_parse(LomoPlayer *self);
void     lomo_player_set_auto_parse(LomoPlayer *self, gboolean auto_parse);

gboolean lomo_player_get_auto_play(LomoPlayer *self);
void     lomo_player_set_auto_play(LomoPlayer *self, gboolean auto_play);

void lomo_player_hook_add(LomoPlayer *self, LomoPlayerHook func, gpointer data);
void lomo_player_hook_remove(LomoPlayer *self, LomoPlayerHook func);

LomoStream *lomo_player_get_stream(LomoPlayer *self);

// Quick play functions, simple shortcuts.
gboolean lomo_player_play_uri(LomoPlayer *self, gchar *uri, GError **error); // API Changed
gboolean lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error); // API Changed

LomoStateChangeReturn lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error);

/**
 * lomo_player_play:
 * @p: The #LomoPlayer
 * @error: Location to store error (if any)
 *
 * Sets #LOMO_STATE_PLAY (start playback) on @p
 *
 * Returns: #TRUE on success, #FALSE otherwise
 */
#define lomo_player_play(p,error)  lomo_player_set_state(p,LOMO_STATE_PLAY, error)

/**
 * lomo_player_pause:
 * @p: The #LomoPlayer
 * @error: Location to store error (if any)
 *
 * Sets #LOMO_STATE_PAUSE (pause playback) on @p
 *
 * Returns: #TRUE on success, #FALSE otherwise
 */
#define lomo_player_pause(p,error) lomo_player_set_state(p,LOMO_STATE_PAUSE,error)

/**
 * lomo_player_stop:
 * @p: The #LomoPlayer
 * @error: Location to store error (if any)
 *
 * Sets #LOMO_STATE_STOP (stop playback) on @p
 *
 * Returns: #TRUE on success, #FALSE otherwise
 */
#define lomo_player_stop(p,error)  lomo_player_set_state(p,LOMO_STATE_STOP, error)

LomoState lomo_player_get_state(LomoPlayer *self);

gint64  lomo_player_tell(LomoPlayer *self, LomoFormat format);
#define lomo_player_tell_time(p)    lomo_player_tell(p,LOMO_FORMAT_TIME)
#define lomo_player_tell_percent(p) lomo_player_tell(p,LOMO_FORMAT_PERCENT)

gboolean lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val);
#define  lomo_player_seek_time(c,t)    lomo_player_seek(c,LOMO_FORMAT_TIME,t)
#define  lomo_player_seek_percent(c,p) lomo_player_seek(c,LOMO_FORMAT_PERCENT,p) // Br0ken

gint64 lomo_player_length(LomoPlayer *self, LomoFormat format);
#define lomo_player_length_time(c)    lomo_player_length(c,LOMO_FORMAT_TIME)
#define lomo_player_length_percent(c) lomo_player_length(c,LOMO_FORMAT_PERCENT) // Br0ken

// Volume and mute
gboolean lomo_player_set_volume(LomoPlayer *self, gint val);
gint lomo_player_get_volume(LomoPlayer *self);

gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute);
gboolean lomo_player_get_mute(LomoPlayer *self);

void    lomo_player_insert(LomoPlayer *self, LomoStream *stream, gint pos);
#define lomo_player_append(p,s)              lomo_player_insert(p,s,-1)
#define lomo_player_insert_uri(p,u,i)        lomo_player_insert(p,lomo_stream_new(i), i)
#define lomo_player_append_uri(p,u)          lomo_player_insert(p,lomo_stream_new(u),-1)

void    lomo_player_insert_multi    (LomoPlayer *self, GList *streams, gint pos);
void    lomo_player_insert_uri_strv (LomoPlayer *self, gchar **uris, gint pos);
void    lomo_player_insert_uri_multi(LomoPlayer *self, GList *uris, gint pos);
#define lomo_player_append_multi(p,l)     lomo_player_insert_multi(p,l,-1)
#define lomo_player_append_uri_strv(p,v)  lomo_player_insert_uri_strv(p,v,-1)
#define lomo_player_append_uri_multi(p,l) lomo_player_insert_uri_multi(p,l,-1)

gboolean lomo_player_del(LomoPlayer *self, gint pos);

const    GList *lomo_player_get_playlist(LomoPlayer *self);

#define lomo_player_queue_stream(self,stream) lomo_player_queue(self,lomo_player_index(self,stream))
gint    lomo_player_queue       (LomoPlayer *self, gint pos);

#define  lomo_player_dequeue_stream(self,stream) lomo_player_queue_index(self,stream)
gboolean lomo_player_dequeue       (LomoPlayer *self, gint queue_pos);

gint lomo_player_queue_index(LomoPlayer *self, LomoStream *stream);
LomoStream *lomo_player_queue_nth(LomoPlayer *self, guint queue_pos);

void lomo_player_queue_clear(LomoPlayer *self);

gint lomo_player_get_prev(LomoPlayer *self);
gint lomo_player_get_next(LomoPlayer *self);

LomoStream *lomo_player_nth_stream(LomoPlayer *self, gint pos);
gint        lomo_player_index(LomoPlayer *self, LomoStream *stream);

gboolean lomo_player_go_nth(LomoPlayer *self, gint pos, GError **error);
#define  lomo_player_go_prev(p,e) lomo_player_go_nth(p,lomo_player_get_prev(p),e)
#define  lomo_player_go_next(p,e) lomo_player_go_nth(p,lomo_player_get_next(p),e)
gint     lomo_player_get_current(LomoPlayer *self);
#define  lomo_player_get_current_stream(p) lomo_player_nth_stream(p, lomo_player_get_current(p))

guint lomo_player_get_total(LomoPlayer *self);

void lomo_player_clear(LomoPlayer *self);

void     lomo_player_set_repeat(LomoPlayer *self, gboolean val);
gboolean lomo_player_get_repeat(LomoPlayer *self);
void     lomo_player_set_random(LomoPlayer *self, gboolean val);
gboolean lomo_player_get_random(LomoPlayer *self);

void lomo_player_randomize(LomoPlayer *self);

void lomo_player_print_pl(LomoPlayer *self);
void lomo_player_print_random_pl(LomoPlayer *self);

gint64 lomo_player_stats_get_stream_time_played(LomoPlayer *self);

G_END_DECLS

#endif /* _LOMO_PLAYER */
