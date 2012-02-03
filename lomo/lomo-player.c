#include "lomo-player.h"
#include <glib/gi18n.h>
#include <gel/gel.h>
#include "lomo/lomo-playlist.h"
#include "lomo/lomo-metadata-parser.h"
#include "lomo/lomo-em-art-provider.h"
#include "lomo/lomo-stats.h"
#include "lomo/lomo-util.h"
#include "lomo/lomo-logger.h"
#include "lomo-marshallers.h"

#define DEBUG 0
#define DEBUG_PREFIX "LomoPlayer"
#if DEBUG
#	define debug(...) g_debug(DEBUG_PREFIX " " __VA_ARGS__)
#else
#	define debug(...) ;
#endif

G_DEFINE_TYPE (LomoPlayer, lomo_player, G_TYPE_OBJECT)

struct _LomoPlayerPrivate {
	GHashTable *options;

	LomoPlayerVTable    vtable;
	LomoPlaylist       *playlist;
	GQueue             *queue;
	LomoMetadataParser *meta;
	LomoStats          *stats;
	LomoEMArtProvider  *art;

	GList *hooks, *hooks_data;

	GstElement *pipeline;

	gint     volume;
	gboolean mute;

	gboolean auto_parse, auto_play;
	gboolean gapless_mode;

	gboolean in_gapless_transition;
	LomoState _shadow_state;
};

enum {
	PROPERTY_STATE = 1,
	PROPERTY_CURRENT,
	PROPERTY_POSITION,
	PROPERTY_VOLUME,
	PROPERTY_MUTE,
	PROPERTY_RANDOM,
	PROPERTY_REPEAT,
	PROPERTY_AUTO_PARSE,
	PROPERTY_AUTO_PLAY,
	PROPERTY_CAN_GO_PREVIOUS,
	PROPERTY_CAN_GO_NEXT,
	PROPERTY_GAPLESS_MODE
};

enum {
	SEEK,
	CLEAR,
	QUEUE_CLEAR,
	INSERT,
	REMOVE,
	QUEUE,
	DEQUEUE,
	EOS,

	TAG,
	ALL_TAGS,
	ERROR,

	/* E-API */
	PRE_CHANGE,
	CHANGE,

	#ifdef LOMO_PLAYER_E_API
	REPEAT,
	RANDOM,
	STATE_CHANGED,
	PLAY,
	PAUSE,
	STOP,
	VOLUME,
	MUTE,
	#endif

	LAST_SIGNAL
};
guint player_signals[LAST_SIGNAL] = { 0 };

/*
 * Default functions for LomoPlayer vtable
 */
static GstElement*
set_uri(GstElement *old_pipeline, const gchar *uri, GHashTable *opts);
static GstStateChangeReturn
set_state(GstElement *pipeline, GstState state);
static GstState
get_state(GstElement *pipeline);
static gboolean
set_position(GstElement *pipeline, GstFormat format, gint64 position);
static gboolean
get_position(GstElement *pipeline, GstFormat *format, gint64 *position);
static gboolean
get_length(GstElement *pipeline, GstFormat *format, gint64 *duration);
static gboolean
set_volume(GstElement *pipeline, gint volume);

/**
 * LomoStateEnumType
 */
#define LOMO_STATE_ENUM_TYPE (lomo_state_enum_type())
GType
lomo_state_enum_type(void)
{
	static GType etype = 0;
	if (etype == 0)
	{
		static const GEnumValue values[] =
		{
			{ LOMO_STATE_INVALID,  "LOMO_STATE_INVALID",  "invalid"  },
			{ LOMO_STATE_STOP,     "LOMO_STATE_STOP",     "stop"     },
			{ LOMO_STATE_PLAY,     "LOMO_STATE_PLAY",     "play"     },
			{ LOMO_STATE_PAUSE,    "LOMO_STATE_PAUSE",    "pause"    },
			{ LOMO_STATE_N_STATES, "LOMO_STATE_N_STATES", "n-states" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("LomoStateEnumType", values);
	}
	return etype;
}

#define check_method_or_return(self,method)                          \
	G_STMT_START {                                                   \
		if (self->priv->vtable.method == NULL)                       \
		{                                                            \
			g_warning(N_("Missing method %s"), G_STRINGIFY(method)); \
			return;                                                  \
		}                                                            \
	} G_STMT_END

#define check_method_or_return_val(self,method,val,error)                \
	G_STMT_START {                                                       \
		if (self->priv->vtable.method == NULL)                           \
		{                                                                \
			error != NULL ?                                              \
				g_set_error(error, player_quark(), LOMO_PLAYER_ERROR_MISSING_METHOD, \
					N_("Missing method %s"), G_STRINGIFY(method))        \
				:                                                        \
				g_warning(N_("Missing method %s"), G_STRINGIFY(method)); \
			return val;                                                  \
		}                                                                \
	} G_STMT_END

#define check_method_or_warn(self,method) \
	G_STMT_START {                                                       \
		if (self->priv->vtable.method == NULL)                           \
				g_warning(N_("Missing method %s"), G_STRINGIFY(method)); \
	} G_STMT_END

static void     player_set_shadow_state    (LomoPlayer *self, LomoState state);
static void     player_emit_state_change(LomoPlayer *self);
static gboolean player_run_hooks(LomoPlayer *self, LomoPlayerHookType type, gpointer ret, ...);

static void     meta_tag_cb     (LomoMetadataParser *parser, LomoStream *stream, const gchar *tag, LomoPlayer *self);
static void     meta_all_tags_cb(LomoMetadataParser *parser, LomoStream *stream, LomoPlayer *self);
static void     about_to_finish_cb(GstElement *pipeline, LomoPlayer *self);
static gboolean player_bus_watcher(GstBus *bus, GstMessage *message, LomoPlayer *self);

#ifdef LOMO_PLAYER_E_API
static void     player_notify_cb(LomoPlayer *self, GParamSpec *pspec, gpointer user_data);
#endif

GEL_DEFINE_QUARK_FUNC(player)

static void
player_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	LomoPlayer *self = LOMO_PLAYER(object);

	switch (property_id)
	{
	case PROPERTY_STATE:
		g_value_set_enum(value, lomo_player_get_state(self));
		break;

	case PROPERTY_CURRENT:
		g_value_set_int(value, lomo_player_get_current(self));
		break;

	case PROPERTY_POSITION:
		g_value_set_int64(value, lomo_player_get_position(self));
		break;

	case PROPERTY_RANDOM:
		g_value_set_boolean(value, lomo_player_get_random(self));
		break;

	case PROPERTY_REPEAT:
		g_value_set_boolean(value, lomo_player_get_repeat(self));
		break;

	case PROPERTY_VOLUME:
		g_value_set_int(value, lomo_player_get_volume(self));
		break;

	case PROPERTY_MUTE:
		g_value_set_boolean(value, lomo_player_get_mute(self));
		break;

	case PROPERTY_AUTO_PARSE:
		g_value_set_boolean(value, lomo_player_get_auto_parse(self));
		break;

	case PROPERTY_AUTO_PLAY:
		g_value_set_boolean(value, lomo_player_get_auto_play(self));
		break;

	case PROPERTY_CAN_GO_NEXT:
		g_value_set_boolean(value, lomo_player_get_can_go_next(self));
		break;

	case PROPERTY_CAN_GO_PREVIOUS:
		g_value_set_boolean(value, lomo_player_get_can_go_previous(self));
		break;

	case PROPERTY_GAPLESS_MODE:
		g_value_set_boolean(value, lomo_player_get_gapless_mode(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
player_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	LomoPlayer *self = LOMO_PLAYER(object);
	GError *error = NULL;

	switch (property_id)
	{
	case PROPERTY_STATE:
		if (!lomo_player_set_state(self, g_value_get_enum(value), &error))
		{
			g_warning("%s (PROPERTY_STATE): %s", __FUNCTION__, error->message);
			g_error_free(error);
		}
		break;

	case PROPERTY_CURRENT:
		lomo_player_set_current(self, g_value_get_int(value), NULL);
		break;

	case PROPERTY_POSITION:
		lomo_player_set_position(self, g_value_get_int64(value));
		break;

	case PROPERTY_RANDOM:
		lomo_player_set_random(self, g_value_get_boolean(value));
		break;

	case PROPERTY_REPEAT:
		lomo_player_set_repeat(self, g_value_get_boolean(value));
		break;

	case PROPERTY_VOLUME:
		lomo_player_set_volume(self, g_value_get_int(value));
		break;

	case PROPERTY_MUTE:
		lomo_player_set_mute(self, g_value_get_boolean(value));
		break;

	case PROPERTY_AUTO_PARSE:
		lomo_player_set_auto_parse(self, g_value_get_boolean(value));
		break;

	case PROPERTY_AUTO_PLAY:
		lomo_player_set_auto_play(self, g_value_get_boolean(value));
		break;

	case PROPERTY_GAPLESS_MODE:
		lomo_player_set_gapless_mode(self, g_value_get_boolean(value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
player_dispose (GObject *object)
{
	LomoPlayer *self = LOMO_PLAYER(object);
	LomoPlayerPrivate *priv = self->priv;

	gel_object_free_and_invalidate(priv->pipeline);
	gel_object_free_and_invalidate(priv->meta);
	gel_object_free_and_invalidate(priv->art);

	gel_free_and_invalidate(priv->queue,   NULL, g_queue_free);
	gel_free_and_invalidate(priv->options, NULL, g_hash_table_destroy);

	gel_object_free_and_invalidate(priv->playlist);
	gel_object_free_and_invalidate(priv->stats);

	G_OBJECT_CLASS (lomo_player_parent_class)->dispose (object);
}

static void
lomo_player_class_init (LomoPlayerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoPlayerPrivate));

	object_class->get_property = player_get_property;
	object_class->set_property = player_set_property;
	object_class->dispose      = player_dispose;

	/*
	 *
	 * Basic signals
	 *
	 */

	/**
	 * LomoPlayer::seek:
	 * @lomo: the object that received the signal
	 * @from: Position before the seek
	 * @to: Position after the seek
	 *
	 * Emitted when #LomoPlayer seeks in a stream
	 */
	player_signals[SEEK] =
		g_signal_new ("seek",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, seek),
			    NULL, NULL,
			    lomo_marshal_VOID__INT64_INT64,
			    G_TYPE_NONE,
			    2,
				G_TYPE_INT64,
				G_TYPE_INT64);
	/**
	 * LomoPlayer::eos:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when current stream reaches end of stream, think about end of
	 * file for file descriptors.
	 */
	player_signals[EOS] =
		g_signal_new ("eos",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, eos),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	/**
	 * LomoPlayer::insert:
	 * @lomo: the object that received the signal
	 * @stream: #LomoStream object that was added
	 * @position: position of the stream in the playlist
	 *
	 * Emitted when a #LomoStream is inserted into #LomoPlayer
	 */
	player_signals[INSERT] =
		g_signal_new ("insert",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, insert),
			    NULL, NULL,
			    lomo_marshal_VOID__OBJECT_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_OBJECT,
				G_TYPE_INT);
	/**
	 * LomoPlayer::remove:
	 * @lomo: the object that received the signal
	 * @stream: #LomoStream object that was removed
	 * @position: last position of the stream in the playlist
	 *
	 * Emitted when a #LomoStream is remove from #LomoPlayer
	 */
	player_signals[REMOVE] =
		g_signal_new ("remove",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, remove),
			NULL, NULL,
			lomo_marshal_VOID__OBJECT_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_OBJECT,
			G_TYPE_INT);
	/**
	 * LomoPlayer::queue:
	 * @lomo: the object that received the signal
	 * @stream: #LomoStream object that was queued
	 * @position: position of the stream in the queue
	 *
	 * Emitted when a #LomoStream is queued inside a #LomoPlayer
	 */
	player_signals[QUEUE] =
		g_signal_new ("queue",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, queue),
			NULL, NULL,
			lomo_marshal_VOID__OBJECT_INT_INT,
			G_TYPE_NONE,
			3,
			G_TYPE_OBJECT,
			G_TYPE_INT,
			G_TYPE_INT);
	/**
	 * LomoPlayer::dequeue:
	 * @lomo: the object that received the signal
	 * @stream: #LomoStream object that was removed
	 * @position: last position of the stream in the queue
	 *
	 * Emitted when a #LomoStream is dequeue inside a #LomoPlayer
	 */
	player_signals[DEQUEUE] =
		g_signal_new ("dequeue",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, dequeue),
			NULL, NULL,
			lomo_marshal_VOID__OBJECT_INT_INT,
			G_TYPE_NONE,
			3,
			G_TYPE_OBJECT,
			G_TYPE_INT,
			G_TYPE_INT);
	/**
	 * LomoPlayer::clear:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when the playlist of a #LomoPlayer is cleared
	 */
	player_signals[CLEAR] =
		g_signal_new ("clear",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, clear),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/**
	 * LomoPlayer::queue-clear:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when the queue of a #LomoPlayer is cleared
	 */
	player_signals[QUEUE_CLEAR] =
		g_signal_new ("queue-clear",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, queue_clear),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/*
	 *
	 * E-API Signals (change not sure)
	 *
	 */

	/**
	 * LomoPlayer::pre-change:
	 * @lomo: the object that received the signal
	 * @from: active stream at the current instant
	 * @to: active stream after the change will be completed
	 *
	 * Emitted before an change event
	 * <note><para>
	 * This signal is deprecated, use #LomoPlayerHook as a replacement
	 * </para></note>
	 */
	player_signals[PRE_CHANGE] =
		g_signal_new ("pre-change",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, pre_change),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/**
	 * LomoPlayer::change:
	 * @lomo: the object that received the signal
	 * @from: Active element before the change
	 * @to: Active element after the change
	 *
	 * Emitted when the active element in the internal playlist changes
	 */
	player_signals[CHANGE] =
		g_signal_new ("change",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, change),
			NULL, NULL,
			lomo_marshal_VOID__INT_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_INT,
			G_TYPE_INT);
	/**
	 * LomoPlayer::tag:
	 * @lomo: the object that received the signal
	 * @stream: (type LomoStream): #LomoStream that gives a new tag
	 * @tag: Discoved tag
	 *
	 * Emitted when some tag is found in a stream.
	 */
	player_signals[TAG] =
		g_signal_new ("tag",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, tag),
			NULL, NULL,
			lomo_marshal_VOID__OBJECT_STRING,
			G_TYPE_NONE,
			2,
			G_TYPE_OBJECT,
			G_TYPE_STRING);
	/**
	 * LomoPlayer::all-tags:
	 * @lomo: the object that received the signal
	 * @stream: #LomoStream which discoved all his tags
	 *
	 * Emitted when all (or no more to be discoverd) tag are found for a
	 * #LomoStream
	 */
	player_signals[ALL_TAGS] =
		g_signal_new ("all-tags",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, all_tags),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE,
			1,
			G_TYPE_OBJECT);
	/**
	 * LomoPlayer::error:
	 * @lomo: the object that received the signal
	 * @error: error ocurred
	 *
	 * Emitted when some error was ocurred.
	 */
	player_signals[ERROR] =
		g_signal_new ("error",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, error),
			NULL, NULL,
			lomo_marshal_VOID__OBJECT_POINTER,
			G_TYPE_NONE,
			2,
			G_TYPE_OBJECT,
			G_TYPE_POINTER);

	#ifdef LOMO_PLAYER_E_API
	/**
	 * LomoPlayer::state-changed:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when #LomoPlayer changes his state.
	 */
	player_signals[STATE_CHANGED] =
		g_signal_new ("state-changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, state_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/**
	 * LomoPlayer::play:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when #LomoPlayer changes his state to LOMO_STATE_PLAY
	 */
	player_signals[PLAY] =
		g_signal_new ("play",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, play),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/**
	 * LomoPlayer::pause:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when #LomoPlayer changes his state to LOMO_STATE_PAUSE
	 */
	player_signals[PAUSE] =
		g_signal_new ("pause",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, pause),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/**
	 * LomoPlayer::stop:
	 * @lomo: the object that received the signal
	 *
	 * Emitted when #LomoPlayer changes his state to LOMO_STATE_STOP
	 */
	player_signals[STOP] =
		g_signal_new ("stop",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, play),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	/**
	 * LomoPlayer::volume:
	 * @lomo: the object that received the signal
	 * @volume: New value of volume (between 0 and 100)
	 *
	 * Emitted when #LomoPlayer changes his volume
	 */
	player_signals[VOLUME] =
		g_signal_new ("volume",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, volume),
			NULL, NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT);
	/**
	 * LomoPlayer::mute:
	 * @lomo: the object that received the signal
	 * @mute: Current value state of mute
	 *
	 * Emitted when #LomoPlayer mutes or unmutes
	 */
	player_signals[MUTE] =
		g_signal_new ("mute",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (LomoPlayerClass, mute),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__BOOLEAN,
		    G_TYPE_NONE,
		    1,
			G_TYPE_BOOLEAN);
	/**
	 * LomoPlayer::repeat:
	 * @lomo: the object that received the signal
	 * @repeat: value of repeat behaviour
	 *
	 * Emitted when value of the repeat behaviour changes
	 */
	player_signals[REPEAT] =
		g_signal_new ("repeat",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, repeat),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOOLEAN,
			G_TYPE_NONE,
			1,
			G_TYPE_BOOLEAN);
	/**
	 * LomoPlayer::random:
	 * @lomo: the object that received the signal
	 * @repeat: value of random behaviour
	 *
	 * Emitted when value of the random behaviour changes
	 */
	player_signals[RANDOM] =
		g_signal_new ("random",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoPlayerClass, random),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOOLEAN,
			G_TYPE_NONE,
			1,
			G_TYPE_BOOLEAN);
	#endif

	/*
	 *
	 * Basic properties
	 *
	 */

	/**
	 * LomoPlayer:auto-parse:
	 *
	 * Control if #LomoPlayer must parse automatically all inserted streams
	 */
	g_object_class_install_property(object_class, PROPERTY_AUTO_PARSE,
		g_param_spec_boolean("auto-parse", "auto-parse", "Auto parse added streams",
		TRUE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:auto-play:
	 *
	 * Control if #LomoPlayer should auto start play when any stream is
	 * available
	 */
	g_object_class_install_property(object_class, PROPERTY_AUTO_PLAY,
		g_param_spec_boolean("auto-play", "auto-play", "Auto play added streams",
		FALSE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:current:
	 *
	 * Current active stream
	 */
	g_object_class_install_property(object_class, PROPERTY_CURRENT,
		g_param_spec_int("current", "current", "Current stream",
		-1, G_MAXINT, -1, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:state:
	 *
	 * State of the player
	 */
	g_object_class_install_property(object_class, PROPERTY_STATE,
		g_param_spec_enum("state", "state", "Player state",
		LOMO_STATE_ENUM_TYPE, LOMO_STATE_STOP, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:position:
	 *
	 * Position within the stream
	 */
	g_object_class_install_property(object_class, PROPERTY_POSITION,
		g_param_spec_int64("position", "position", "Player position",
		-1, G_MAXINT64, 0, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:volume:
	 *
	 * Volume value, from 0 to 100.
	 **/
	g_object_class_install_property(object_class, PROPERTY_VOLUME,
		g_param_spec_int("volume", "volume", "Volume",
		0, 100, 50, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:mute:
	 *
	 * Mute value, %TRUE or %FALSE
	 **/
	g_object_class_install_property(object_class, PROPERTY_MUTE,
		g_param_spec_boolean("mute", "mute", "Mute",
		FALSE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:random:
	 *
	 * Enables or disables random mode
	 **/
	g_object_class_install_property(object_class, PROPERTY_RANDOM,
		g_param_spec_boolean("random", "random", "Random",
		FALSE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:repeat:
	 *
	 * Enables or disables repeat mode
	 **/
	g_object_class_install_property(object_class, PROPERTY_REPEAT,
		g_param_spec_boolean("repeat", "repeat", "Repeat",
		FALSE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:can-go-previous:
	 *
	 * Check if player can backwards in playlist
	 */
	g_object_class_install_property(object_class, PROPERTY_CAN_GO_PREVIOUS,
		g_param_spec_boolean("can-go-previous", "can-go-previous", "Can go previous",
		FALSE, G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:can-go-next:
	 *
	 * Check if player can forward in playlist
	 */
	g_object_class_install_property(object_class, PROPERTY_CAN_GO_NEXT,
		g_param_spec_boolean("can-go-next", "can-go-next", "Can go next",
		FALSE, G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
	/**
	 * LomoPlayer:gapless
	 *
	 * Enable or disable gapless mode
	 */
	g_object_class_install_property(object_class, PROPERTY_GAPLESS_MODE,
		g_param_spec_boolean("gapless-mode", "gapless-mode", "Gapless mode",
		TRUE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));
}

static void
lomo_player_init (LomoPlayer *self)
{
	LomoPlayerPrivate *priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), LOMO_TYPE_PLAYER, LomoPlayerPrivate);
	LomoPlayerVTable vtable = {
		set_uri,

		set_state,
		get_state,

		set_position,
		get_position,
		get_length,

		set_volume,
		NULL, // get volume,

		NULL, // set mute
		NULL  // get mute
	};

	priv->vtable   = vtable;
	priv->options  = g_hash_table_new(g_str_hash, g_str_equal);
	priv->pipeline = NULL;
	priv->volume   = 50;
	priv->mute     = FALSE;
	priv->playlist = lomo_playlist_new();
	priv->meta     = lomo_metadata_parser_new();
	priv->art      = lomo_em_art_provider_new();
	priv->queue    = g_queue_new();
	priv->stats    = lomo_stats_new(self);

	// Shadow values
	priv->_shadow_state     = LOMO_STATE_INVALID;

	g_signal_connect(priv->meta, "tag",      (GCallback) meta_tag_cb, self);
	g_signal_connect(priv->meta, "all-tags", (GCallback) meta_all_tags_cb, self);

	#ifdef LOMO_PLAYER_E_API
	g_signal_connect(self, "notify", (GCallback) player_notify_cb, NULL);
	#endif

	const gchar *lomo_debug_env = g_getenv("LIBLOMO_DEBUG");
	if (lomo_debug_env && g_str_equal(lomo_debug_env, "1"))
		lomo_player_start_logger(self);

	lomo_em_art_provider_set_player(priv->art, self);
}

/**
 * lomo_player_new:
 * @option_name: First option name
 * @Varargs: A %NULL-terminated pairs of option-name,option-value
 *
 * Creates a new #LomoPlayer with options
 *
 * Returns: the new created #LomoPlayer
 */
LomoPlayer*
lomo_player_new (gchar *option_name, ...)
{
	LomoPlayer        *self = g_object_new (LOMO_TYPE_PLAYER, NULL);
	LomoPlayerPrivate *priv = self->priv;

	va_list args;
	va_start(args, option_name);

	while (option_name != NULL)
	{
		gchar *value = va_arg(args, gchar*);
		g_hash_table_replace(priv->options, (gpointer) option_name, (gpointer) value);

		option_name = va_arg(args, gchar*);
	}
	va_end(args);

	if (g_hash_table_lookup(priv->options, "audio-output") == NULL)
	{
		g_warning("audio-output option is mandatory while using lomo_player_new_with_opts\n");
		g_object_unref(self);
		return NULL;
	}

	return self;
}

/**
 * lomo_player_get_auto_parse:
 * @self: a #LomoPlayer
 *
 * Gets the auto-parse property value.
 *
 * Returns: The auto-parse property value.
 */
gboolean
lomo_player_get_auto_parse(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return self->priv->auto_parse;
}

/**
 * lomo_player_set_auto_parse:
 * @self: a #LomoPlayer
 * @auto_parse: new value for auto-parse property
 *
 * Sets the auto-parse property value.
 */
void
lomo_player_set_auto_parse(LomoPlayer *self, gboolean auto_parse)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	if (self->priv->auto_parse != auto_parse)
	{
		self->priv->auto_parse = auto_parse;
		g_object_notify(G_OBJECT(self), "auto-parse");
	}
}

/**
 * lomo_player_get_auto_play:
 * @self: a #LomoPlayer
 *
 * Gets the auto-play property value.
 *
 * Returns: The auto-play property value.
 */
gboolean
lomo_player_get_auto_play(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return self->priv->auto_play;
}

/**
 * lomo_player_set_auto_play:
 * @self: a #LomoPlayer
 * @auto_play: new value for auto-play property
 *
 * Sets the auto-play property value.
 */
void
lomo_player_set_auto_play(LomoPlayer *self, gboolean auto_play)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	if (self->priv->auto_play != auto_play)
	{
		self->priv->auto_play = auto_play;
		g_object_notify(G_OBJECT(self), "auto-play");
	}
}

/**
 * lomo_player_get_gapless_mode:
 * @self: A #LomoPlayer
 *
 * Returns whether the gapless mode is active or not
 *
 * Returns: %TRUE if gapless mode is enabled
 */
gboolean
lomo_player_get_gapless_mode(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return self->priv->gapless_mode;
}

/**
 * lomo_player_set_gapless_mode:
 * @self: A #LomoPlayer
 * @gapless_mode: Whatever the gapless mode has to be actived or not
 *
 * Enables or disables the gapless mode
 */
void
lomo_player_set_gapless_mode(LomoPlayer *self, gboolean gapless_mode)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));

	LomoPlayerPrivate *priv = self->priv;
	if (priv->gapless_mode == gapless_mode)
		return;

	priv->gapless_mode = gapless_mode;

	/* Apply setting on pipeline */
	if (priv->pipeline)
	{
		if (priv->gapless_mode)
			g_signal_connect(priv->pipeline, "about-to-finish", (GCallback) about_to_finish_cb, self);
		else
			g_signal_handlers_disconnect_by_func(priv->pipeline, about_to_finish_cb, self);
	}

	g_object_notify((GObject *) self, "gapless-mode");
}

/**
 * lomo_player_get_state:
 * @self: The #LomoPlayer
 *
 * Gets current state from @self
 *
 * Returns: a #LomoState representing current state
 */
LomoState
lomo_player_get_state(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), LOMO_STATE_INVALID);
	check_method_or_return_val(self, get_state, LOMO_STATE_INVALID, NULL);

	LomoPlayerPrivate *priv = self->priv;

	if (priv->_shadow_state != LOMO_STATE_INVALID)
		return  priv->_shadow_state;

	if (!priv->pipeline || (lomo_player_get_current(self)) == -1)
		return LOMO_STATE_STOP;

	GstState gst_state = priv->vtable.get_state(priv->pipeline);

	LomoState state;
	if (!lomo_state_from_gst(gst_state, &state))
		return LOMO_STATE_INVALID;

	return state;
}


/**
 * lomo_player_set_state:
 * @self: The #LomoPlayer
 * @state: The #LomoState to set
 * @error: Location to store error (if any)
 *
 * Changes state of @self to @state.
 *
 * Returns: %TRUE if successful, %FALSE otherwise
 */
gboolean
lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	check_method_or_return_val(self, set_state, FALSE, error);

	LomoPlayerPrivate *priv = self->priv;

	// Dont update state twice
	if (state == lomo_player_get_state(self))
		return TRUE;

	if (priv->pipeline == NULL)
	{
		if ((state = LOMO_STATE_STOP) || (state == LOMO_STATE_INVALID))
		{
			player_emit_state_change(self);
			return TRUE;
		}
		else
		{
			g_set_error(error, player_quark(), LOMO_PLAYER_ERROR_MISSING_PIPELINE,
				N_("Cannot set state: missing pipeline"));
			return FALSE;
		}
	}

	GstState gst_state;
	if (!lomo_state_to_gst(state, &gst_state))
	{
		g_set_error(error, player_quark(), LOMO_PLAYER_ERROR_UNKNOW_STATE,
			"Unknow state '%d'", state);
		return FALSE;
	}


	GstStateChangeReturn ret = priv->vtable.set_state(priv->pipeline, gst_state);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		g_set_error(error, player_quark(), LOMO_PLAYER_ERROR_SET_STATE,
			N_("Error setting state"));
		return FALSE;
	}

	// GST_STATE_ASYNC is catched on bus_watch
	if (ret == GST_STATE_CHANGE_SUCCESS)
		player_emit_state_change(self);

	return TRUE;
}

/**
 * lomo_player_get_current:
 * @self: A #LomoPlayer
 *
 * Gets the index for the current active stream in the player
 *
 * Returns: the index
 */
gint
lomo_player_get_current(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), -1);
	return lomo_playlist_get_current(self->priv->playlist);
}

/**
 * lomo_player_set_current
 * @self: An #LomoPlayer
 * @index: Index
 * @error: Location for errors
 *
 * Gets the current active item
 *
 * Returns: %TRUE on successfull, %FALSE otherwise
 */
gboolean
lomo_player_set_current(LomoPlayer *self, gint index, GError **error)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	check_method_or_return_val(self, set_uri,  FALSE, error);

	LomoPlayerPrivate *priv = self->priv;

	gint old_index = lomo_player_get_current(self);

	if (priv->in_gapless_transition)
	{
		lomo_playlist_set_current(priv->playlist, index);
		g_signal_emit(self, player_signals[CHANGE], 0, old_index, index);
		g_object_notify((GObject *) self, "current");
		/*
		g_object_notify((GObject *) self, "can-go-next");
		g_object_notify((GObject *) self, "can-go-previous");
		*/
		return TRUE;
	}

	gboolean ret = FALSE;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_CHANGE, &ret, old_index, index))
	{
		if (!ret)
			g_set_error(error, player_quark(), LOMO_PLAYER_ERROR_BLOCK_BY_HOOK,
				_("Action blocked by hook"));
		return ret;
	}

	/*
	 * Check and fix index value
	 */
	if (index >= lomo_player_get_n_streams(self))
	{
		g_warning("Index is overlimit");
		index = (lomo_player_get_n_streams(self) > 0) ? 0 : -1;
	}

	// Nothing to do here?
	if (old_index == index)
	{
		if (index != -1)
			g_warn_if_fail(old_index != index);
		return TRUE;
	}

	// Check if new index is -1 and delete everything
	if (index == -1)
	{
		debug("Going to -1...");
		if (priv->pipeline != NULL)
		{
			debug("  nuking pipeline");
			lomo_player_set_state(self, LOMO_STATE_STOP, NULL);
			priv->vtable.set_state(priv->pipeline, GST_STATE_NULL);
			g_object_unref(priv->pipeline);
			priv->pipeline = NULL;
		}
		lomo_playlist_set_current(priv->playlist, -1);
		g_object_notify((GObject *) self, "current");
		/*
		g_object_notify((GObject *) self, "can-go-previous");
		g_object_notify((GObject *) self, "can-go-next");
		*/
		g_signal_emit(self, player_signals[CHANGE], 0, old_index, -1);
		return TRUE;
	}

	debug("Going to %d...", index);

	// Check stream, this should never happend
	LomoStream *stream = lomo_player_get_nth_stream(self, index);
	if (!LOMO_IS_STREAM(stream))
	{
		debug("  stream is fucked, reboot _set_current");
		g_warn_if_fail(LOMO_IS_STREAM(stream));
		lomo_player_set_current(self, -1, NULL);
		return FALSE;
	}

	// Save state for restore it later
	GstState state = GST_STATE_READY;
	if (!priv->vtable.get_state)
	{
		check_method_or_warn(self, get_state);
		state = GST_STATE_READY;
	}
	else if (priv->pipeline)
		state = priv->vtable.get_state(priv->pipeline);

	// Check for a reusable pipeline
	GstElement *new_pipeline = priv->vtable.set_uri(
		priv->pipeline,
		lomo_stream_get_uri(stream),
		priv->options);

	// Old pipeline is not reusable
	if (new_pipeline != priv->pipeline)
	{
		// Wipe it
		if (priv->pipeline)
		{
			debug("Old pipeline is going to be wiped");
			check_method_or_warn(self, set_state);
			if (priv->vtable.set_state)
				priv->vtable.set_state(priv->pipeline, GST_STATE_NULL);
			g_object_unref(priv->pipeline);
		}

		// Assume new pipeline
		debug("new pipeline is assumed and default values assigned");
		priv->pipeline = new_pipeline;
		lomo_player_set_volume(self, -1);         // Restore pipeline volume
		lomo_player_set_mute  (self, priv->mute); // Restore pipeline mute
		gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(new_pipeline)), (GstBusFunc) player_bus_watcher, self);
		if (priv->gapless_mode)
			g_signal_connect(new_pipeline, "about-to-finish", (GCallback) about_to_finish_cb, self);
	}

	// Set URI stream on the pipeline
	if (!GST_IS_PIPELINE(priv->pipeline))
	{
		g_warn_if_fail(GST_IS_PIPELINE(priv->pipeline));
		lomo_player_set_current(self, -1, NULL);
		return FALSE;
	}

	// Restore state
	if (!priv->vtable.set_state)
	{
		check_method_or_warn(self, set_state);
		lomo_player_set_current(self, -1, NULL);
		return FALSE;
	}

	// Check queue stuff
	if (stream)
	{
		gint queue_index = lomo_player_queue_get_stream_index(self, stream);
		if (queue_index >= 0)
			lomo_player_dequeue(self, queue_index);
	}

	debug("Everything ok, fire notifies");
	LomoState lomo_state = LOMO_STATE_STOP;
	if (lomo_state_from_gst(state, &lomo_state))
		player_set_shadow_state(self, lomo_state);
	lomo_playlist_set_current(priv->playlist, index);
	g_object_notify((GObject *) self, "current");
	/*
	g_object_notify((GObject *) self, "can-go-previous");
	g_object_notify((GObject *) self, "can-go-next");
	*/
	player_set_shadow_state(self, LOMO_STATE_INVALID);
	priv->vtable.set_state(priv->pipeline, state);
	if (old_index == -1)
		g_object_notify((GObject *) self, "state");
	g_signal_emit(self, player_signals[CHANGE], 0, old_index, index);

	return TRUE;
}

/**
 * lomo_player_get_volume:
 * @self: a #LomoPlayer
 *
 * Gets current volume (between 0 and 100)
 *
 * Returns: current volume
 */
gint
lomo_player_get_volume(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), -1);

	LomoPlayerPrivate *priv = self->priv;
	if (priv->pipeline == NULL)
		return priv->volume;

	if (priv->vtable.get_volume == NULL)
		return priv->volume;

	gint ret = priv->vtable.get_volume(priv->pipeline);
	if (ret == -1)
		g_return_val_if_fail((ret >= 0) && (ret <= 100), -1);

	return ret;
}

/**
 * lomo_player_set_volume:
 * @self: a #LomoPlayer
 * @val: new value for volume
 *
 * Sets volume, val must be between 0 and 100
 *
 * Returns: %TRUE on success, %FALSE if an error ocurred
 */
gboolean
lomo_player_set_volume(LomoPlayer *self, gint val)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	g_warn_if_fail((val >= -1) && (val <= 100));

	check_method_or_return_val(self, set_volume, FALSE, NULL);
	LomoPlayerPrivate *priv = self->priv;

	if (val == -1)
		val = priv->volume;
	val = CLAMP(val, 0, 100);

	if (priv->pipeline == NULL)
	{
		priv->volume = val;
		g_object_notify(G_OBJECT(self), "volume");
		return TRUE;
	}

	// Call hooks
	gboolean ret = FALSE;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_VOLUME, &ret, val))
		return ret;

	// Exec action
	if (!priv->vtable.set_volume(priv->pipeline, val))
	{
		g_warning(N_("Error setting volume"));
		return FALSE;
	}

	priv->volume = val;
	g_object_notify(G_OBJECT(self), "volume");

	return TRUE;
}

/**
 * lomo_player_get_mute:
 * @self: a #LomoPlayer
 *
 * Gets current mute value
 *
 * Returns: current mute value
 */
gboolean
lomo_player_get_mute(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);

	LomoPlayerPrivate *priv = self->priv;
	if (priv->vtable.get_mute)
		return priv->vtable.get_mute(priv->pipeline);
	else
		return priv->mute;
}

/**
 * lomo_player_set_mute:
 * @self: a #LomoPlayer
 * @mute: new value for mute
 *
 * Sets mute
 *
 * Returns: %TRUE on success, %FALSE if an error ocurred
 */
gboolean
lomo_player_set_mute(LomoPlayer *self, gboolean mute)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	LomoPlayerPrivate *priv = self->priv;

	if ((priv->vtable.set_mute == NULL) && (priv->vtable.set_volume == NULL))
	{
		g_warning(N_("Missing set_mute and set_volume methods, cannot mute"));
		return FALSE;
	}

	// Run hooks
	gboolean ret = FALSE;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_MUTE, &ret, mute))
		return ret;

	if (priv->pipeline == NULL)
	{
		priv->mute = mute;
		g_object_notify(G_OBJECT(self), "mute");
		return TRUE;
	}

	// Exec action
	if (priv->pipeline == NULL)
	{
		priv->mute = mute;
		g_object_notify(G_OBJECT(self), "mute");
		ret = TRUE;
	}
	else
	{
		if (priv->vtable.set_mute)
			ret = priv->vtable.set_mute(priv->pipeline, mute);
		else
			ret = priv->vtable.set_volume(priv->pipeline, mute ? 0 : priv->volume);
	}

	if (!ret)
		g_return_val_if_fail(ret, ret);

	priv->mute = mute;
	g_object_notify(G_OBJECT(self), "mute");

	return ret;
}

/**
 * lomo_player_get_repeat:
 * @self: a #LomoPlayer
 *
 * Gets current value of the repeat behaviour
 *
 * Returns: %TRUE if repeat is applied, %FALSE otherwise
 */
gboolean
lomo_player_get_repeat(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return lomo_playlist_get_repeat(self->priv->playlist);
}

/**
 * lomo_player_set_repeat:
 * @self: a #LomoPlayer
 * @val: Value for repeat behaviour
 *
 * Sets the value of the repeat behaviour
 */
void
lomo_player_set_repeat(LomoPlayer *self, gboolean val)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));

	// Run hook
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_REPEAT, NULL, val))
		return;

	// Exec action
	lomo_playlist_set_repeat(self->priv->playlist, val);

	g_object_notify(G_OBJECT(self), "repeat");
	g_object_notify(G_OBJECT(self), "can-go-previous");
	g_object_notify(G_OBJECT(self), "can-go-next");
}

/**
 * lomo_player_get_random:
 * @self: a #LomoPlayer
 *
 * Gets current value of the random behaviour
 *
 * Returns: %TRUE if repeat is applied, %FALSE otherwise
 */
gboolean lomo_player_get_random(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return lomo_playlist_get_random(self->priv->playlist);
}

/**
 * lomo_player_set_random:
 * @self: a #LomoPlayer
 * @val: Value for random behaviour
 *
 * Sets the value of the random behaviour, if @value is %TRUE playlist will be
 * randomized
 */
void
lomo_player_set_random(LomoPlayer *self, gboolean val)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));

	// Run hook
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_RANDOM, NULL, val))
		return;

	// Exec action
	lomo_playlist_set_random(self->priv->playlist, val);

	g_object_notify(G_OBJECT(self), "random");
	g_object_notify(G_OBJECT(self), "can-go-previous");
	g_object_notify(G_OBJECT(self), "can-go-next");
}

/**
 * lomo_player_get_nth_stream:
 * @self: a #LomoPlayer
 * @index: the index of the #LomoStream, starting from 0
 *
 * Gets the #LomoStream at the given position
 *
 * Returns: (transfer none): the #LomoStream, or %NULL if @index is off the list
 */
LomoStream*
lomo_player_get_nth_stream(LomoPlayer *self, gint index)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), NULL);

	if (index == -1)
		return NULL;
	g_return_val_if_fail((index >= -1) || (index > (lomo_player_get_n_streams(self) - 1)), NULL);

	return lomo_playlist_get_nth_stream(self->priv->playlist, index);
}

/**
 * lomo_player_get_previous:
 * @self: a #LomoPlayer
 *
 * Gets the position of the previous stream in the playlist following
 * random and repeat behaviours
 *
 * Returns: the position of the previous stream, or -1 if none
 */
gint lomo_player_get_previous(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return lomo_playlist_get_previous(self->priv->playlist);
}

/**
 * lomo_player_get_next:
 * @self: a #LomoPlayer
 *
 * Gets the position of the next stream in the playlist following
 * random and repeat behaviours
 *
 * Returns: the position of the next stream, or -1 if none
 */
gint lomo_player_get_next(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);

	LomoPlayerPrivate *priv = self->priv;
	LomoStream *stream = g_queue_peek_head(priv->queue);
	if (stream)
		return lomo_playlist_get_stream_index(priv->playlist, stream);
	else
		return lomo_playlist_get_next(priv->playlist);
}

/**
 * lomo_player_get_can_go_previous:
 * @self: A #LomoPlayer
 *
 * Gets the value of the 'can-go-previous' property
 *
 * Returns: The value
 */
gboolean
lomo_player_get_can_go_previous(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return (lomo_player_get_previous(self) >= 0);
}

/**
 * lomo_player_get_can_go_next:
 * @self: A #LomoPlayer
 *
 * Gets the value of the 'can-go-next' property
 *
 * Returns: The value
 */
gboolean
lomo_player_get_can_go_next(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	return (lomo_player_get_next(self) >= 0);
}

gint64
lomo_player_get_position(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);

	check_method_or_return_val(self, get_position, -1, NULL);
	LomoPlayerPrivate *priv = self->priv;

	if (!priv->pipeline || (lomo_player_get_current(self) < 0))
		return -1;

	GstFormat gst_format = GST_FORMAT_TIME;
	gint64 ret;
	if (!priv->vtable.get_position(priv->pipeline, &gst_format, &ret))
		return -1;
	g_return_val_if_fail(gst_format == GST_FORMAT_TIME, -1);

	return ret;
}

gboolean
lomo_player_set_position(LomoPlayer *self, gint64 position)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	g_return_val_if_fail(position >= 0, FALSE);

	check_method_or_return_val(self, set_position, FALSE, NULL);
	LomoPlayerPrivate *priv = self->priv;

	if ((priv->pipeline == NULL) || (lomo_player_get_current(self) < 0))
	{
		g_warning(N_("Player not seekable"));
		return FALSE;
	}

	/* XXX: Is this really needed? */
	gint64 old_pos = lomo_player_get_position(self);
	if (old_pos == -1)
		return FALSE;

	gboolean ret = FALSE;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_SEEK, &ret, old_pos, position))
		return ret;

	// Exec action
	ret = priv->vtable.set_position(priv->pipeline, GST_FORMAT_TIME, position);
	if (ret)
		g_signal_emit(G_OBJECT(self), player_signals[SEEK], 0, old_pos, position);
	else
		g_warning(N_("Error seeking"));

	g_object_notify(G_OBJECT(self), "position");
	return ret;
}

gint64
lomo_player_get_length(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);

	check_method_or_return_val(self, get_length, -1, NULL);
	LomoPlayerPrivate *priv = self->priv;

	if ((priv->pipeline == NULL) || (lomo_player_get_current(self) < 0))
		return -1;

	GstFormat gst_format = GST_FORMAT_TIME;
	gint64 ret;
	if (!priv->vtable.get_length(priv->pipeline, &gst_format, &ret))
		return -1;
	g_return_val_if_fail(gst_format == GST_FORMAT_TIME, -1);

	return ret;
}


/**
 * lomo_player_insert:
 * @self: a #LomoPlayer
 * @stream: (transfer none): a #LomoStream which will be owner by @self
 * @index: position to insert the element, If this is negative, or is larger than
 *       the number of elements in the list, the new element is added on to the end
 *       of the list.
 *
 * Inserts a #LomoStream in the internal playlist
 */
void
lomo_player_insert(LomoPlayer *self, LomoStream *stream, gint index)
{
	GList *tmp = g_list_prepend(NULL, stream);
	lomo_player_insert_multiple(self, tmp, index);
	g_list_free(tmp);
}

/**
 * lomo_player_insert_uri:
 * @self: a #LomoPlayer
 * @uri: a URI
 * @index: position to insert the element, If this is negative, or is larger than
 *       the number of elements in the list, the new element is added on to the end
 *       of the list.
 *
 * Inserts a #LomoStream in the internal playlist
 */
void
lomo_player_insert_uri(LomoPlayer *self, const gchar *uri, gint index)
{
	const gchar *tmp[] = { uri, NULL };
	lomo_player_insert_strv(self, tmp, index);
}

/**
 * lomo_player_insert_strv:
 * @self: a #LomoPlayer
 * @uris: (transfer none) (array zero-terminated=1) (element-type utf8): a
 *        NULL-terminated array of URIs
 * @index: Index to insert the elements, If this is negative, or is
 *         larger than the number of elements in the list, the new elements
 *         are added on to the end of the list.
 *
 * Inserts multiple URIs into @self
 */
void
lomo_player_insert_strv(LomoPlayer *self, const gchar *const *uris, gint index)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	g_return_if_fail(uris);

	gchar *tmp;
	GList *streams = NULL;
	for (guint i = 0; uris[i] != NULL; i++)
	{
		if ((tmp = g_uri_parse_scheme(uris[i])) != NULL)
		{
			g_free(tmp);
			streams = g_list_prepend(streams, lomo_stream_new(uris[i]));
		}
		else if ((tmp = g_filename_to_uri(uris[i], NULL, NULL)) != NULL)
		{
			streams = g_list_prepend(streams, lomo_stream_new(tmp));
			g_free(tmp);
		}
		else
			g_warning(_("Invalid URI or path: '%s'"), uris[i]);
	}
	streams = g_list_reverse(streams);
	lomo_player_insert_multiple(self, streams, index);
	g_list_foreach(streams, (GFunc) g_object_unref, NULL);
	g_list_free(streams);
}

/**
 * lomo_player_insert_multiple:
 * @self: a #LomoPlayer
 * @streams: (element-type Lomo.Stream) (transfer none): a #GList of #LomoStream which will be owned by @self
 * @index: Index to insert the elements, If this is negative, or is larger
 *         than the number of elements in the list, the new elements are added on to the
 *         end of the list.
 *
 * Inserts multiple streams in the internal playlist
 */
void
lomo_player_insert_multiple(LomoPlayer *self, GList *streams, gint position)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));

	// Fix position
	gint n_streams = lomo_player_get_n_streams(self);
	if ((position < 0) || (position > n_streams))
		position = n_streams;

	// Emit change after adding
	gboolean emit_change = (n_streams == 0);

	GList *l = streams;
	while (l)
	{
		LomoStream *stream = LOMO_STREAM(l->data);
		if (!LOMO_IS_STREAM(stream))
		{
			g_warn_if_fail(LOMO_IS_STREAM(stream));
			l = l->next;
			continue;
		}

		// Run hook
		if (player_run_hooks(self, LOMO_PLAYER_HOOK_INSERT, NULL, stream, position))
		{
			l = l->next;
			continue;
		}

		// Insert and auto-parse
		lomo_playlist_insert(self->priv->playlist, stream, position++);
		g_signal_emit(self, player_signals[INSERT], 0, stream, position - 1);
		if (lomo_player_get_auto_parse(self))
			lomo_metadata_parser_parse(self->priv->meta, stream, LOMO_METADATA_PARSER_PRIO_DEFAULT);

		// Fire change after the first insert

		l = l->next;
	}

	// emit change
	if (emit_change                     &&
	    lomo_player_get_n_streams(self) &&
	    !player_run_hooks(self, LOMO_PLAYER_HOOK_CHANGE, NULL, -1, 0))
	{
		GError *err = NULL;

		// Force playlist to -1
		lomo_playlist_set_current(self->priv->playlist, -1);
		if (!lomo_player_set_current(self, 0, &err))
		{
			g_warning(N_("Error creating pipeline: %s"), err->message);
			g_error_free(err);
		}

		if (lomo_player_get_auto_play(self))
			lomo_player_set_state(self, LOMO_STATE_PLAY, NULL);
	}

	// prev and next could have changed if any stream was added
	if (n_streams != lomo_player_get_n_streams(self))
	{
		g_object_notify((GObject *) self, "can-go-previous");
		g_object_notify((GObject *) self, "can-go-next");
	}
}

/**
 * lomo_player_remove:
 * @self: a #LomoPlayer
 * @index: Index of the #LomoStream to remove
 *
 * Removes a #LomoStream from the internal playlist using and index
 * This also implies a reference decrease on the element
 *
 * Returns: %TRUE is @position was remove, %FALSE otherwise
 */
gboolean
lomo_player_remove(LomoPlayer *self, gint index)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	g_return_val_if_fail(lomo_player_get_n_streams(self) > index, FALSE);

	LomoPlayerPrivate *priv = self->priv;
	gint curr, next;

	LomoStream *stream = lomo_player_get_nth_stream(self, index);

	// Call hooks
	gboolean ret = FALSE;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_REMOVE, &ret, stream, index))
		return ret;

	// Exec action
	g_object_ref(stream);
	curr = lomo_player_get_current(self);

	if (curr == index)
	{
		next = lomo_player_get_next(self);
		if ((next == curr) || (next == -1))
		{
			// mmm, only one stream, go stop
			lomo_player_set_state(self, LOMO_STATE_STOP, NULL);
			lomo_player_set_current(self, -1, NULL);
		}
		else
			/* Delete and go next */
			lomo_player_set_current(self, lomo_player_get_next(self), NULL);
	}

	lomo_playlist_remove(priv->playlist, index);

	g_signal_emit(G_OBJECT(self), player_signals[REMOVE], 0, stream, index);
	g_object_notify((GObject *) self, "can-go-previous");
	g_object_notify((GObject *) self, "can-go-next");
	g_object_unref(stream);

	return TRUE;
}

/**
 * lomo_player_get_playlist:
 * @self: a #LomoPlayer
 *
 * Gets current playlist
 *
 * Returns: (element-type Lomo.Stream) (transfer none): a #GList of #LomoStream. Should not be modified
 */
const GList *
lomo_player_get_playlist(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), NULL);
	return lomo_playlist_get_playlist(self->priv->playlist);
}

gint
lomo_player_get_n_streams(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), 0);
	return lomo_playlist_get_n_streams(self->priv->playlist);
}

gint
lomo_player_get_stream_index(LomoPlayer *self, LomoStream *stream)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), -1 );
	g_return_val_if_fail(LOMO_IS_STREAM(stream), -1);

	return lomo_playlist_get_stream_index(self->priv->playlist, stream);
}

void
lomo_player_clear(LomoPlayer *self)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	LomoPlayerPrivate *priv = self->priv;

	if (lomo_player_get_current(self) == -1)
		return;

	// Run hook
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_CLEAR, NULL))
		return;

	// Exec action
	// lomo_player_set_state(self, LOMO_STATE_STOP, NULL);
	lomo_player_set_current(self, -1, NULL); // This will also set stop

	lomo_playlist_clear(priv->playlist);
	lomo_metadata_parser_clear(priv->meta);

	/*
	g_object_notify((GObject *) self, "can-go-next");
	g_object_notify((GObject *) self, "can-go-previous");
	*/
	g_object_notify((GObject *) self, "current");

	g_signal_emit(G_OBJECT(self), player_signals[CLEAR], 0);
}

/**
 * lomo_player_queue:
 * @self: a #LomoPlayer
 * @index: position of the element to queue
 *
 * Queues the element for inmediate play after active element
 *
 * Returns: index of the elemement in the queue
 */
gint
lomo_player_queue(LomoPlayer *self, gint index)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), -1);
	g_return_val_if_fail((index >= 0) && (index < lomo_player_get_n_streams(self)), -1);
	LomoPlayerPrivate *priv = self->priv;

	LomoStream *stream = lomo_playlist_get_nth_stream(priv->playlist, index);
	g_return_val_if_fail(stream != NULL, -1);

	// Run hooks
	gint ret = -1;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_QUEUE, &ret, stream, g_queue_get_length(priv->queue)))
		return ret;

	// Exec action
	g_queue_push_tail(priv->queue, stream);
	gint queue_index = g_queue_get_length(priv->queue) - 1;

	g_signal_emit(G_OBJECT(self), player_signals[QUEUE], 0, stream, index, queue_index);
	return queue_index;
}

/**
 * lomo_player_dequeue:
 * @self: a #LomoPlayer
 * @queue_index: Index of the queue to dequeue
 *
 * Removes the element indicated by @queue_pos from the queue
 *
 * Returns: %TRUE if element was dequeue, %FALSE if @queue_pos was invalid
 */
gboolean
lomo_player_dequeue(LomoPlayer *self, gint queue_index)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	LomoPlayerPrivate *priv = self->priv;
	LomoStream *stream = g_queue_peek_nth(priv->queue, queue_index);
	g_return_val_if_fail(stream != NULL, FALSE);

	// Run hooks
	gboolean ret = FALSE;
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_DEQUEUE, &ret, stream, queue_index))
		return ret;

	gint index = lomo_player_get_stream_index(self, stream);
	g_signal_emit(G_OBJECT(self), player_signals[DEQUEUE], 0, stream, index, queue_index);

	// Exec action
	if (g_queue_pop_nth(priv->queue, queue_index) == NULL)
		return FALSE;

	return TRUE;
}

/**
 * lomo_player_queue_get_n_streams:
 * @self: A #LomoPlayer
 *
 * Returns the number of streams in the queue
 *
 * Returns: Number of streams
 */
gint
lomo_player_queue_get_n_streams(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	LomoPlayerPrivate *priv = self->priv;

	return g_queue_get_length(priv->queue);
}

/**
 * lomo_player_queue_index:
 * @self: a #LomoPlayer
 * @stream: a #LomoStream to find in queue
 *
 * Gets the position of the #LomoStream in the queue (starting from 0).
 *
 * Returns: the index of the #LomoStream in the queue, or -1 if the #LomoStream
 * is not found in the queue
 */
gint
lomo_player_queue_get_stream_index(LomoPlayer *self, LomoStream *stream)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), -1);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), -1);

	return g_queue_index(self->priv->queue, stream);
}

/**
 * lomo_player_queue_get_nth_stream:
 * @self: a #LomoPlayer
 * @queue_index: The queue index of the element, starting from 0
 *
 * Gets the #LomoStream at the given position in the queue
 *
 * Returns: (transfer none): the #LomoStream, or %NULL if the position is off the end of the
 *          queue
 */
LomoStream *
lomo_player_queue_get_nth_stream(LomoPlayer *self, gint queue_index)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), NULL);
	g_return_val_if_fail((queue_index >= 0) && (queue_index < lomo_player_queue_get_n_streams(self)), NULL);

	return g_queue_peek_nth(self->priv->queue, queue_index);
}

/**
 * lomo_player_queue_clear:
 * @self: a #LomoPlayer
 *
 * Removes all #LomoStream queued
 */
void
lomo_player_queue_clear(LomoPlayer *self)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));

	// Run hooks
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_QUEUE_CLEAR, NULL))
		return;

	// Exec action
	g_queue_clear(self->priv->queue);
	g_signal_emit(G_OBJECT(self), player_signals[QUEUE_CLEAR], 0);
}

/**
 * lomo_player_hook_add:
 * @self: a #LomoPlayer
 * @func: (scope call): a #LomoPlayerHook function
 * @data: data to pass to @func or NULL to ignore
 *
 * Add a hook to the hook system
 */
void
lomo_player_hook_add(LomoPlayer *self, LomoPlayerHook func, gpointer data)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	g_return_if_fail(func != NULL);

	LomoPlayerPrivate *priv = self->priv;
	priv->hooks      = g_list_prepend(priv->hooks, func);
	priv->hooks_data = g_list_prepend(priv->hooks_data, data);
}

/**
 * lomo_player_hook_remove:
 * @self: a #LomoPlayer
 * @func: (scope call): a #LomoPlayerHook function
 *
 * Remove a hook from the hook system
 */
void
lomo_player_hook_remove(LomoPlayer *self, LomoPlayerHook func)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	g_return_if_fail(func != NULL);

	LomoPlayerPrivate *priv = self->priv;

	gint index = g_list_index(priv->hooks, func);
	g_return_if_fail(index >= 0);

	GList *p = g_list_nth(priv->hooks, index);
	priv->hooks = g_list_delete_link(priv->hooks, p);

	p = g_list_nth(priv->hooks_data, index);
	priv->hooks_data = g_list_delete_link(priv->hooks_data, p);
}

/**
 * lomo_player_stats_get_stream_time_played:
 * @self: a #LomoPlayer
 *
 * Returns: Microseconds stream was played
 */
gint64
lomo_player_stats_get_stream_time_played(LomoPlayer *self)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), -1);
	return lomo_stats_get_time_played(self->priv->stats);
}


// --
// Private funcs
// --
static void
player_set_shadow_state (LomoPlayer *self, LomoState state)
{
	g_return_if_fail(LOMO_IS_PLAYER(self));
	self->priv->_shadow_state = state;
}

static void
player_emit_state_change(LomoPlayer *self)
{
	static LomoState prev = LOMO_STATE_INVALID;
	LomoState curr = lomo_player_get_state(self);
	if (prev != curr)
	{
		g_object_notify(G_OBJECT(self), "state");
		prev = curr;
	}
}

static gboolean
player_run_hooks(LomoPlayer *self, LomoPlayerHookType type, gpointer ret, ...)
{
	LomoPlayerPrivate *priv = self->priv;

	GList *iter = priv->hooks;
	GList *iter2 = priv->hooks_data;

	if (iter == NULL)
		return FALSE;

	LomoPlayerHookEvent event = { .type = type };
	va_list args;
	va_start(args, ret);
	switch (type)
	{
	case LOMO_PLAYER_HOOK_SEEK:
		event.old = va_arg(args,gint64);
		event.new = va_arg(args,gint64);
		break;

	case LOMO_PLAYER_HOOK_VOLUME:
		event.volume = va_arg(args, gint);
		break;

	case LOMO_PLAYER_HOOK_MUTE:
	case LOMO_PLAYER_HOOK_REPEAT:
	case LOMO_PLAYER_HOOK_RANDOM:
		event.value = va_arg(args, gboolean);
		break;

	case LOMO_PLAYER_HOOK_INSERT:
	case LOMO_PLAYER_HOOK_REMOVE:
		event.stream = va_arg(args, LomoStream*);
		event.pos    = va_arg(args, gint);
		break;

	case LOMO_PLAYER_HOOK_QUEUE:
	case LOMO_PLAYER_HOOK_DEQUEUE:
		event.stream = va_arg(args, LomoStream*);
		event.queue_pos = va_arg(args, gint);
		break;

	case LOMO_PLAYER_HOOK_PLAY:
	case LOMO_PLAYER_HOOK_PAUSE:
	case LOMO_PLAYER_HOOK_STOP:
	case LOMO_PLAYER_HOOK_CLEAR:
	case LOMO_PLAYER_HOOK_QUEUE_CLEAR:
	case LOMO_PLAYER_HOOK_EOS:
		break;

	case LOMO_PLAYER_HOOK_CHANGE:
		event.from = va_arg(args, gint);
		event.to   = va_arg(args, gint);
		break;

	case LOMO_PLAYER_HOOK_TAG:
		event.stream = va_arg(args, LomoStream*);
		event.tag    = va_arg(args, const gchar*);
		break;

	case LOMO_PLAYER_HOOK_ALL_TAGS:
		event.stream = va_arg(args, LomoStream*);
		break;

	case LOMO_PLAYER_HOOK_ERROR:
		event.error = va_arg(args, GError*);
		break;

	// Use #if 0 to catch unhandled hooks at compile time
	#if 1
	default:
		g_warning("Hook " G_STRINGIFY(type) " not recognized");
		va_end(args);
		return FALSE;
	#endif
	}
	va_end(args);

	gboolean stop = FALSE;
	while (iter && (stop == FALSE))
	{
		LomoPlayerHook func = (LomoPlayerHook) iter->data;
		stop  = func(self, event, ret, iter2->data);
		iter  = iter->next;
		iter2 = iter2->next;
	}

	return stop;
}

static void
meta_tag_cb(LomoMetadataParser *parser, LomoStream *stream, const gchar *tag, LomoPlayer *self)
{
	// Run hook
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_TAG, NULL, stream, tag))
		return;

	// Exec signal
	g_signal_emit(self, player_signals[TAG], 0, stream, tag);
}

static void
meta_all_tags_cb(LomoMetadataParser *parser, LomoStream *stream, LomoPlayer *self)
{
	// Run hook
	if (player_run_hooks(self, LOMO_PLAYER_HOOK_ALL_TAGS, NULL, stream))
		return;

	// Exec action
	g_signal_emit(self, player_signals[ALL_TAGS], 0, stream);
}

#if 0
gboolean
analize_element_msg_cb(GQuark field_id, const GValue *value, LomoPlayer *self)
{
	return FALSE;
	static GQuark q = 0;
	if (!q)
		q = g_quark_from_static_string("uri");
	if (q == field_id)
	{
		debug("Got URI");
		gint64 pos = lomo_player_get_position(self);
		gint64 len =  lomo_player_get_length(self);
		gint64 b;
		g_object_get(self->priv->pipeline, "buffer-duration", &b, NULL);
		debug("Pos.: %"G_GINT64_FORMAT", len: %"G_GINT64_FORMAT"", len, pos);
		pos += b;
		debug("Pos.: %"G_GINT64_FORMAT", len: %"G_GINT64_FORMAT"", len, pos);
		return FALSE;
	}
	/*
	gchar *v =   g_strdup_value_contents(value);
	debug("  %s: %s", g_quark_to_string(field_id), v);
	g_free(v);
	*/
	return TRUE;
}

static void
print_stats(LomoPlayer *self)
{
	gint64 pos = lomo_player_get_position(self) / 1000000L;
	gint64 len = lomo_player_get_length(self)   / 1000000L;

	gint64 bsize;
	g_object_get(self->priv->pipeline, "buffer-duration", &bsize, NULL);

	debug(" Current diff: %"G_GINT64_FORMAT" milisecs, buffer len: %"G_GINT64_FORMAT"", len - pos, bsize);
}
#endif

static gboolean
player_bus_watcher(GstBus *bus, GstMessage *message, LomoPlayer *self)
{
	GError *err = NULL;
	gchar *debug = NULL;
	LomoStream *stream = NULL;
	gint next;

	LomoPlayerPrivate *priv = self->priv;

	/*
	debug("Got msg %s", GST_MESSAGE_TYPE_NAME(message));
	print_stats(self);

	if (message->structure)
		gst_structure_foreach(message->structure, struct_cb, NULL);
	*/
	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(message, &err, &debug);
			stream = lomo_player_get_current_stream(self);

			// Call hook
			if (player_run_hooks(self, LOMO_PLAYER_HOOK_ERROR, NULL, stream, err))
			{
				g_error_free(err);
				g_free(debug);
				break;
			}

			// Exec action
			if (stream != NULL)
				lomo_stream_set_failed_flag(stream, TRUE);

			g_signal_emit(G_OBJECT(self), player_signals[ERROR], 0, stream, err);
			g_error_free(err);
			g_free(debug);
			break;

		case GST_MESSAGE_EOS:
			if (player_run_hooks(self, LOMO_PLAYER_HOOK_EOS, NULL))
				break;

			g_signal_emit(G_OBJECT(self), player_signals[EOS], 0);
			next = lomo_player_get_next(self);
			if (next == -1)
			{
				lomo_player_set_state(self, LOMO_STATE_STOP, NULL);
				lomo_player_set_current(self, 0, NULL);
			}
			else
				lomo_player_set_current(self, next, NULL);
			break;

		case GST_MESSAGE_STATE_CHANGED:
		{
			GstState oldstate, newstate, pending;

			gst_message_parse_state_changed(message, &oldstate, &newstate, &pending);
			#if 0
			g_warning("Got state change from bus: old=%s, new=%s, pending=%s\n",
				lomo_gst_state_to_str(oldstate),
				lomo_gst_state_to_str(newstate),
				lomo_gst_state_to_str(pending));
			#endif

			if (pending != GST_STATE_VOID_PENDING)
				break;

			switch (newstate)
			{
			case GST_STATE_NULL:
			case GST_STATE_READY:
				break;
			case GST_STATE_PAUSED:
				/* Ignore pause events before 50 miliseconds, gstreamer pauses
				 * pipeline after a first play event, returning to play state
				 * inmediatly.
				 */
				if ((lomo_player_get_position(self) / 1000000) <= 50)
					return TRUE;
				break;
			case GST_STATE_PLAYING:
				break;
			default:
				g_warning("ERROR: Unknow state transition: %s\n", lomo_gst_state_to_str(newstate));
				return TRUE;
			}

			player_emit_state_change(self);
			break;
		}

		case GST_MESSAGE_ELEMENT:
			/* Pipeline can send GstMessageElement but it is only relevant
			 * if gapless mode is active */
			if (!priv->gapless_mode)
				break;

			/* Handle this message only in gapless transition */
			if (!priv->in_gapless_transition)
				break;

			/* Jump to next element in the same way we do in GST_MESSAGE_EOS */
			next = lomo_player_get_next(self);
			if (next == -1)
			{
				lomo_player_set_state(self, LOMO_STATE_STOP, NULL);
				lomo_player_set_current(self, 0, NULL);
			}
			else
				lomo_player_set_current(self, next, NULL);

			break;

		// Messages that can be ignored
		case GST_MESSAGE_TAG: /* Handled */
		case GST_MESSAGE_NEW_CLOCK:
			break;

		// Debug this to get more info about this kind of messages
		case GST_MESSAGE_CLOCK_PROVIDE:
		case GST_MESSAGE_CLOCK_LOST:
		case GST_MESSAGE_UNKNOWN:
		case GST_MESSAGE_WARNING:
		case GST_MESSAGE_INFO:
		case GST_MESSAGE_BUFFERING:
		case GST_MESSAGE_STATE_DIRTY:
		case GST_MESSAGE_STEP_DONE:
		case GST_MESSAGE_STRUCTURE_CHANGE:
		case GST_MESSAGE_STREAM_STATUS:
		case GST_MESSAGE_APPLICATION:
		case GST_MESSAGE_SEGMENT_START:
		case GST_MESSAGE_SEGMENT_DONE:
		case GST_MESSAGE_DURATION:
		case GST_MESSAGE_LATENCY:
		case GST_MESSAGE_ASYNC_START:
		case GST_MESSAGE_ASYNC_DONE:
		case GST_MESSAGE_REQUEST_STATE:
		case GST_MESSAGE_STEP_START:
		case GST_MESSAGE_QOS:
		case GST_MESSAGE_ANY:
			break;
		default:
			debug(_("Bus got something like... '%s'"), gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
			break;
		}

	return TRUE;
}

static void
about_to_finish_cb(GstElement *pipeline, LomoPlayer *self)
{
	LomoPlayerPrivate *priv = self->priv;

	// debug("Got about-to-finish");
	// print_stats(self);
	gint next = lomo_player_get_next(self);
	if (next < 0)
		return;

	LomoStream *s = lomo_player_get_nth_stream(self, next);
	g_return_if_fail(LOMO_IS_STREAM(s));

	const gchar *uri = lomo_stream_get_uri(s);
	g_return_if_fail(uri != NULL);

	/* Emit EOS here, I dont know any other method to report EOS
	 * in gapless-mode
	 */
	g_signal_emit(self, player_signals[EOS], 0);

	g_object_set(self->priv->pipeline, "uri", uri, NULL);
	priv->in_gapless_transition = TRUE;
}

#ifdef LOMO_PLAYER_E_API
static void
player_notify_cb(LomoPlayer *self, GParamSpec *pspec, gpointer user_data)
{
	struct {
		gchar *signal_name;
		guint  signal_id;
	} table[] = {
		{ "repeat", REPEAT },
		{ "random", RANDOM },
		{ "volume", VOLUME },
		{ "mute",   MUTE   }
		};

	gchar *ignore[] = {
		"can-go-previous",
		"can-go-next",
		"auto-play",
		"auto-parse",
		"gapless-mode"
		};

	LomoPlayerPrivate *priv = self->priv;

	for (guint i = 0; i < G_N_ELEMENTS(ignore); i++)
		if (g_str_equal(pspec->name, ignore[i]))
			return;

	if (g_str_equal(pspec->name, "state"))
	{
		g_signal_emit(self, player_signals[STATE_CHANGED], 0);
		return;
	}

	if (g_str_equal(pspec->name, "current"))
	{
		/* LomoPlayer::change can also be emitted here but only
		 * if a way to emit it on idle.
		 */

		priv->in_gapless_transition = FALSE;
		g_object_notify((GObject *) self, "can-go-previous");
		g_object_notify((GObject *) self, "can-go-next");

		LomoStream *stream = lomo_player_get_current_stream(self);
		if (stream)
		{
			gint queue_idx = lomo_player_queue_get_stream_index(self, stream);
			if (queue_idx >= 0)
				lomo_player_dequeue(self, queue_idx);
		}

		return;
	}

	for (guint i = 0; i < G_N_ELEMENTS(table); i++)
		if (g_str_equal(pspec->name, table[i].signal_name))
		{
			g_signal_emit((GObject *) self, player_signals[table[i].signal_id], 0);
			return;
		}

	// g_warning(_("Unhanded notify::%s"), pspec->name);
}

#endif

// --
// Default functions for LomoPlayerVTable
// --
static GstElement*
set_uri(GstElement *old_pipeline, const gchar *uri, GHashTable *opts)
{
	if (uri == NULL)
	{
		if (old_pipeline)
			g_object_unref(old_pipeline);
		g_return_val_if_fail(uri != NULL, NULL);
	}

	if (old_pipeline && (get_state(old_pipeline) != GST_STATE_NULL))
		set_state(old_pipeline, GST_STATE_NULL);

	GstElement *ret = old_pipeline ? old_pipeline : gst_element_factory_make("playbin2", "playbin2");
	const gchar *audio_sink_str = (const gchar *) g_hash_table_lookup(opts, (gpointer) "audio-output");
	if (audio_sink_str == NULL)
			audio_sink_str = "autoaudiosink";

	GstElement *audio_sink = gst_element_factory_make(audio_sink_str, "audio-sink");
	if (audio_sink == NULL)
	{
		g_warn_if_fail(GST_IS_ELEMENT(audio_sink));
		g_object_unref(ret);
		return NULL;
	}

	g_object_set(G_OBJECT(ret), "audio-sink", audio_sink, NULL);
	g_object_set(G_OBJECT(ret), "uri", uri, NULL);

	gst_element_set_state(ret, GST_STATE_READY);

	return ret;
}

static GstStateChangeReturn
set_state(GstElement *pipeline, GstState state)
{
	return gst_element_set_state(pipeline, state);
}

static GstState
get_state(GstElement *pipeline)
{
	GstState state, pending;
	gst_element_get_state(pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
	return state;
}

static gboolean
set_position(GstElement *pipeline, GstFormat format, gint64 position)
{
	return gst_element_seek(pipeline, 1.0,
		format, GST_SEEK_FLAG_FLUSH,
		GST_SEEK_TYPE_SET, position,
		GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

static gboolean
get_position(GstElement *pipeline, GstFormat *format, gint64 *position)
{
	return gst_element_query_position(pipeline, format, position);
}

static gboolean
get_length(GstElement *pipeline, GstFormat *format, gint64 *duration)
{
	return gst_element_query_duration(pipeline, format, duration);
}

static gboolean
set_volume(GstElement *pipeline, gint volume)
{
	gdouble v = (gdouble) volume / 100;
	g_object_set(G_OBJECT(pipeline), "volume", v, NULL);
	return TRUE;
}

#ifdef LOMO_PLAYER_COMPAT
gboolean lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	g_return_val_if_fail(format == LOMO_FORMAT_TIME, FALSE);

	return lomo_player_set_position(self, val);
}

gint64
lomo_player_tell(LomoPlayer *self, LomoFormat format)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	g_return_val_if_fail(format == LOMO_FORMAT_TIME, FALSE);

	return lomo_player_get_position(self);
}

gint64
lomo_player_length(LomoPlayer *self, LomoFormat format)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(self), FALSE);
	g_return_val_if_fail(format == LOMO_FORMAT_TIME, FALSE);

	return lomo_player_get_length(self);
}
#endif
