#ifndef __LOMO_PLAYER_H__
#define __LOMO_PLAYER_H__

#include <glib-object.h>
#include <gst/gst.h>
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define LOMO_PLAYER_E_API
#define LOMO_PLAYER_COMPAT

#define LOMO_TYPE_PLAYER lomo_player_get_type()

#define LOMO_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_PLAYER, LomoPlayer))
#define LOMO_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LOMO_TYPE_PLAYER, LomoPlayerClass))
#define LOMO_IS_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_PLAYER)) 
#define LOMO_IS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LOMO_TYPE_PLAYER)) 
#define LOMO_PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LOMO_TYPE_PLAYER, LomoPlayerClass))

typedef struct _LomoPlayerPrivate LomoPlayerPrivate;
typedef struct {
	GObject parent;
	LomoPlayerPrivate *priv;
} LomoPlayer;

typedef struct {
	GObjectClass parent_class;

	void (*seek)          (LomoPlayer *self, gint64 old, gint64 new);
	void (*eos)           (LomoPlayer *self);

	void (*insert)        (LomoPlayer *self, LomoStream *stream, gint64 pos);
	void (*remove)        (LomoPlayer *self, LomoStream *stream, gint64 pos);
	void (*queue)         (LomoPlayer *self, LomoStream *stream, gint64 pos);
	void (*dequeue)       (LomoPlayer *self, LomoStream *stream, gint64 pos);

	void (*clear)         (LomoPlayer *self);
	void (*queue_clear)   (LomoPlayer *self);

	void (*error)         (LomoPlayer *self, LomoStream *stream, GError *error);
	void (*tag)           (LomoPlayer *self, LomoStream *stream, const gchar *tag);
	void (*all_tags)      (LomoPlayer *self, LomoStream *stream);

	/* Maybe E-API */
	void (*pre_change)    (LomoPlayer *self);
	void (*change)        (LomoPlayer *self, gint from, gint to);

	/* E-API */
	void (*repeat)        (LomoPlayer *self, gboolean val);
	void (*random)        (LomoPlayer *self, gboolean val);
	void (*state_changed) (LomoPlayer *self);
	void (*play)          (LomoPlayer *self);
	void (*pause)         (LomoPlayer *self);
	void (*stop)          (LomoPlayer *self);
	void (*volume)        (LomoPlayer *self, gint volume);
	void (*mute)          (LomoPlayer *self, gboolean mute);
} LomoPlayerClass;

/**
 * LomoPlayerVTable:
 * Override default methods from #LomoPlayer
 */
typedef struct {
	GstElement*          (*set_uri)   (GstElement *old_pipeline, const gchar *uri, GHashTable *opts);

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
 * @LOMO_STATE_N_STATES: Helper define
 *
 * Defines the state of the #LomoPlayer object
 **/
typedef enum {
	LOMO_STATE_INVALID = -1,
	LOMO_STATE_STOP    = 0,
	LOMO_STATE_PLAY    = 1,
	LOMO_STATE_PAUSE   = 2,

	LOMO_STATE_N_STATES
} LomoState;

/**
 * LomoFormat:
 * @LOMO_FORMAT_INVALID: Invalid format
 * @LOMO_FORMAT_TIME: Format is time
 * @LOMO_FORMAT_PERCENT: Format is precent
 * @LOMO_FORMAT_N_FORMATS: Helper define
 *
 * Define in which format data is expressed
 **/
typedef enum {
	LOMO_FORMAT_INVALID = -1,
	LOMO_FORMAT_TIME    = 0,
	LOMO_FORMAT_PERCENT = 1,

	LOMO_FORMAT_N_FORMATS
} LomoFormat;

/**
 * LomoPlayerError:
 * @LOMO_PLAYER_ERROR_NO_ERROR: No error
 * @LOMO_PLAYER_ERROR_INVALID_ARGUMENT: One or more arguments are invalid
 * @LOMO_PLAYER_ERROR_NO_STREAM: There is no stream at desired position (soft
 *                               error)
 * @LOMO_PLAYER_ERROR_MISSING_METHOD: Method is not implemented
 * @LOMO_PLAYER_ERROR_CREATE_PIPELINE: Pipeline cannot be created
 * @LOMO_PLAYER_ERROR_MISSING_PIPELINE: Pipeline is missing
 * @LOMO_PLAYER_ERROR_SET_STATE: State cannot be set
 * @LOMO_PLAYER_ERROR_CANNOT_DEQUEUE: Stream cannot be dequeued
 * @LOMO_PLAYER_ERROR_UNKNOW_STATE: Pipeline's state is unknow
 * @LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE: Failure in state change
 * @LOMO_PLAYER_ERROR_HOOK_BLOCK: Action was blocked by a hook
 */
typedef enum {
	LOMO_PLAYER_ERROR_MISSING_METHOD = 1,
	LOMO_PLAYER_ERROR_MISSING_PIPELINE,
	LOMO_PLAYER_ERROR_UNKNOW_STATE,
	LOMO_PLAYER_ERROR_SET_STATE
	#if 0
	LOMO_PLAYER_ERROR_CREATE_PIPELINE,
	LOMO_PLAYER_ERROR_CANNOT_DEQUEUE,
	LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE,
	LOMO_PLAYER_ERROR_NO_STREAM,
	LOMO_PLAYER_ERROR_HOOK_BLOCK,
	LOMO_PLAYER_ERROR_INVALID_ARGUMENT
	#endif
} LomoPlayerError;

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
	const gchar *tag;   // tag
	gboolean value;     // random, repeat, mute
	GError *error;      // error
} LomoPlayerHookEvent;

/**
 * LomoPlayerHook:
 * @self: A #LomoPlayer
 * @event: The #LomoPlayerHookEvent to handle
 * @ret: Alternative value for return
 * @data: User data
 *
 * Handles hook events
 *
 * Returns: %TRUE if event was handle and stop the default implementation,
 *          %FALSE if other hooks and default implementation should be called
 **/
typedef gboolean(*LomoPlayerHook)(LomoPlayer *self, LomoPlayerHookEvent event, gpointer ret, gpointer data);


GType lomo_player_get_type (void);

LomoPlayer* lomo_player_new (gchar *option_name, ...);

/**
 * gets & sets
 */

/* configurations */
gboolean lomo_player_get_auto_parse(LomoPlayer *self);
void     lomo_player_set_auto_parse(LomoPlayer *self, gboolean auto_parse);

gboolean lomo_player_get_auto_play(LomoPlayer *self);
void     lomo_player_set_auto_play(LomoPlayer *self, gboolean auto_play);

/* state */
LomoState lomo_player_get_state(LomoPlayer *self);
gboolean  lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error);

/* current stream */
gint64   lomo_player_get_current(LomoPlayer *self);
gboolean lomo_player_set_current(LomoPlayer *self, gint64 current, GError **error);

/* volume & mute */
gboolean lomo_player_set_volume(LomoPlayer *self, gint val);
gint     lomo_player_get_volume(LomoPlayer *self);

gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute);
gboolean lomo_player_get_mute(LomoPlayer *self);

/* playlist control */
void     lomo_player_set_repeat(LomoPlayer *self, gboolean val);
gboolean lomo_player_get_repeat(LomoPlayer *self);

void     lomo_player_set_random(LomoPlayer *self, gboolean val);
gboolean lomo_player_get_random(LomoPlayer *self);

LomoStream* lomo_player_get_nth_stream(LomoPlayer *self, gint64 index);

gint64 lomo_player_get_previous(LomoPlayer *self);
gint64 lomo_player_get_next    (LomoPlayer *self);

gboolean lomo_player_get_can_go_previous(LomoPlayer *self);
gboolean lomo_player_get_can_go_next    (LomoPlayer *self);

/* API calls */
gint64   lomo_player_get_position(LomoPlayer *self);
gboolean lomo_player_set_position(LomoPlayer *self, gint64 position);
gint64   lomo_player_get_length  (LomoPlayer *self);

void     lomo_player_insert_strv    (LomoPlayer *self, const gchar *const *uris, gint64 position);
void     lomo_player_insert_multiple(LomoPlayer *self, GList *streams, gint64 position);
gboolean lomo_player_remove         (LomoPlayer *self, gint64 index);

const GList* lomo_player_get_playlist    (LomoPlayer *self);
gint64       lomo_player_get_n_streams   (LomoPlayer *self);
gint64       lomo_player_get_stream_index(LomoPlayer *self, LomoStream *stream);
void         lomo_player_randomize       (LomoPlayer *self);
void         lomo_player_clear           (LomoPlayer *self);

// XXX: Queue API will be rewritten
#define     lomo_player_queue_stream(self,stream)   lomo_player_queue(self,lomo_player_get_stream_index(self,stream))
#define     lomo_player_dequeue_stream(self,stream) lomo_player_queue_index(self,stream)
gint64      lomo_player_queue       (LomoPlayer *self, gint64 pos);
gboolean    lomo_player_dequeue     (LomoPlayer *self, gint64 queue_pos);
gint64      lomo_player_queue_index (LomoPlayer *self, LomoStream *stream);
LomoStream* lomo_player_queue_nth   (LomoPlayer *self, gint64 queue_pos);
void        lomo_player_queue_clear (LomoPlayer *self);

void lomo_player_hook_add(LomoPlayer *self, LomoPlayerHook func, gpointer data);
void lomo_player_hook_remove(LomoPlayer *self, LomoPlayerHook func);

gint64 lomo_player_stats_get_stream_time_played(LomoPlayer *self);

#ifdef LOMO_PLAYER_E_API

#define lomo_player_append_strv(self, uris)        lomo_player_insert_strv(self, uris, -1)
#define lomo_player_append_multiple(self, streams) lomo_player_insert_multiple(self, streams, -1)

#endif

#ifdef  LOMO_PLAYER_COMPAT

#define lomo_player_play(self,error)  lomo_player_set_state(self, LOMO_STATE_PLAY,  error)
#define lomo_player_pause(self,error) lomo_player_set_state(self, LOMO_STATE_PAUSE, error)
#define lomo_player_stop(self,error)  lomo_player_set_state(self, LOMO_STATE_STOP,  error)

gboolean lomo_player_seek  (LomoPlayer *self, LomoFormat format, gint64 val);
gint64   lomo_player_tell  (LomoPlayer *self, LomoFormat format);
gint64   lomo_player_length(LomoPlayer *self, LomoFormat format);

#define lomo_player_seek_time(c,t) lomo_player_seek(c,LOMO_FORMAT_TIME,t)
#define lomo_player_tell_time(p)   lomo_player_tell(p,LOMO_FORMAT_TIME)
#define lomo_player_length_time(c) lomo_player_length(c,LOMO_FORMAT_TIME)

#define lomo_player_append_uri_multi(self, uri_list) lomo_player_insert_uri_multi(self,uri_list,-1)
void    lomo_player_insert_uri_multi(LomoPlayer *self, GList *uris, gint64 position);
#define lomo_player_del(self,index) lomo_player_remove(self,index)

#define lomo_player_go_previous(self,error)  lomo_player_set_current(self, lomo_player_get_previous(self), error)
#define lomo_player_go_next(self,error)      lomo_player_set_current(self, lomo_player_get_next(self), error)
#define lomo_player_go_nth(self,index,error) lomo_player_set_current(self,index,error)
#define lomo_player_index(self,stream)       lomo_player_get_stream_index(self,stream)
#define lomo_player_nth_stream(self,index)   lomo_player_get_nth_stream(self,index)
#define lomo_player_get_current_stream(self) lomo_player_get_nth_stream(self, lomo_player_get_current(self))
#define lomo_player_get_total(self)          lomo_player_get_n_streams(self)

#endif

G_END_DECLS

#endif /* __LOMO_PLAYER_H__ */

