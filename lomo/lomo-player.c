/*
 * lomo/lomo-player.c
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
#include <glib/gi18n.h>
#include <gst/gst.h>
#include <lomo/lomo-player.h>
#include <lomo/lomo-playlist.h>
#include <lomo/lomo-metadata-parser.h>
#include <lomo/lomo-marshallers.h>
#include <lomo/lomo-util.h>

struct _LomoPlayerPrivate {
	LomoPlayerVTable  vtable;
	GHashTable       *options;

	GList *hooks, *hooks_data;

	gboolean            auto_parse;

	GstElement         *pipeline;
	LomoPlaylist       *pl;
	GQueue             *queue;
	LomoMetadataParser *meta;
	LomoStream         *stream;

	gint          volume;
	gboolean      mute;
};

enum {
	LOMO_PLAYER_PROPERTY_AUTO_PARSE = 1
};

#define check_method_or_return(self,method)                             \
	G_STMT_START {                                                      \
		if (self->priv->vtable.method == NULL)                          \
		{                                                               \
			g_warning(N_("Missing method %s"), G_STRINGIFY(method));    \
			return;                                                     \
		}                                                               \
	} G_STMT_END

#define check_method_or_return_val(self,method,val,error)                \
	G_STMT_START {                                                       \
		if (self->priv->vtable.method == NULL)                           \
		{                                                                \
			error != NULL ?                                              \
				g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_MISSING_METHOD, \
					N_("Missing method %s"), G_STRINGIFY(method))        \
				:                                                        \
				g_warning(N_("Missing method %s"), G_STRINGIFY(method)); \
			return val;                                                  \
		}                                                                \
	} G_STMT_END

G_DEFINE_TYPE(LomoPlayer, lomo_player, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_PLAYER, LomoPlayerPrivate))

enum {
	PLAY,
	PAUSE,
	STOP,
	SEEK,
	VOLUME,
	MUTE,

	INSERT,
	REMOVE,

	QUEUE,
	DEQUEUE,
	QUEUE_CLEAR,

	PRE_CHANGE,
	CHANGE,
	CLEAR,
	REPEAT,
	RANDOM,
	EOS,
	ERROR,
	TAG,
	ALL_TAGS,

	LAST_SIGNAL
};
guint lomo_player_signals[LAST_SIGNAL] = { 0 };

// --
// Callbacks
// --
static void
tag_cb(LomoMetadataParser *parser, LomoStream *stream, LomoTag tag, LomoPlayer *self);
static void
all_tags_cb(LomoMetadataParser *parser, LomoStream *stream, LomoPlayer *self);
static gboolean
bus_watcher(GstBus *bus, GstMessage *message, LomoPlayer *self); 

// --
// VFuncs
// --
static GstElement*
create_pipeline(const gchar *uri, GHashTable *opts);
static void
destroy_pipeline(GstElement *pipeline);
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

gboolean
lomo_player_create_pipeline(LomoPlayer *self, LomoStream *stream, GError **error);
gboolean
lomo_player_destroy_pipeline(LomoPlayer *self, GError **error);

// --
// LomoPlayer
// --
static GQuark
lomo_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("lomo-player");
	return ret;
}

static void
lomo_player_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	LomoPlayer *self = LOMO_PLAYER(object);
	switch (property_id)
	{
	case LOMO_PLAYER_PROPERTY_AUTO_PARSE:
		g_value_set_boolean(value, lomo_player_get_auto_parse(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
lomo_player_set_property(GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	LomoPlayer *self = LOMO_PLAYER(object);

	switch (property_id)
	{
	case LOMO_PLAYER_PROPERTY_AUTO_PARSE:
		lomo_player_set_auto_parse(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
lomo_player_dispose(GObject *object)
{
	LomoPlayer *self;

	self = LOMO_PLAYER (object);

	if (self->priv->options)
	{
		g_hash_table_unref(self->priv->options);
		self->priv->options = NULL;
	}

	if (self->priv->pipeline)
	{
		if (lomo_player_get_state(self) != LOMO_STATE_STOP)
			lomo_player_stop(self, NULL);
		self->priv->vtable.destroy_pipeline(self->priv->pipeline);
		self->priv->pipeline = NULL;
	}

	if (self->priv->pl)
	{
		lomo_playlist_unref(self->priv->pl);
		self->priv->pl = NULL;
	}

	if (self->priv->queue)
	{
		g_queue_free(self->priv->queue);
		self->priv->queue = NULL;
	}

	if (self->priv->meta)
	{
		g_object_unref(self->priv->meta);
		self->priv->meta = NULL;
	}

	self->priv->stream = NULL;

	G_OBJECT_CLASS (lomo_player_parent_class)->dispose (object);
}

static void
lomo_player_class_init (LomoPlayerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = lomo_player_get_property;
	object_class->set_property = lomo_player_set_property;
	object_class->dispose = lomo_player_dispose;

	// core signals
	lomo_player_signals[PLAY] =
		g_signal_new ("play",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, play),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[PAUSE] =
		g_signal_new ("pause",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, pause),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[STOP] =
		g_signal_new ("stop",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, stop),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[SEEK] =
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
	lomo_player_signals[VOLUME] =
		g_signal_new ("volume",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, volume),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__INT,
			    G_TYPE_NONE,
			    1,
				G_TYPE_INT);
	lomo_player_signals[MUTE] =
		g_signal_new ("mute",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, mute),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__BOOLEAN,
			    G_TYPE_NONE,
			    1,
				G_TYPE_BOOLEAN);

	// playlist signals
	lomo_player_signals[INSERT] =
		g_signal_new ("insert",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, insert),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_POINTER,
				G_TYPE_INT);
	lomo_player_signals[REMOVE] =
		g_signal_new ("remove",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, remove),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_POINTER,
				G_TYPE_INT);

	lomo_player_signals[QUEUE] =
		g_signal_new ("queue",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, queue),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_POINTER,
				G_TYPE_INT);
	lomo_player_signals[DEQUEUE] =
		g_signal_new ("dequeue",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, dequeue),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_POINTER,
				G_TYPE_INT);
	lomo_player_signals[QUEUE_CLEAR] =
		g_signal_new ("queue-clear",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, queue_clear),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[PRE_CHANGE] =
		g_signal_new ("pre-change",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, pre_change),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[CHANGE] =
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
	lomo_player_signals[CLEAR] =
		g_signal_new ("clear",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, clear),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[REPEAT] =
		g_signal_new ("repeat",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, repeat),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__BOOLEAN,
			    G_TYPE_NONE,
			    1,
				G_TYPE_BOOLEAN);
	lomo_player_signals[RANDOM] =
		g_signal_new ("random",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, random),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__BOOLEAN,
			    G_TYPE_NONE,
			    1,
				G_TYPE_BOOLEAN);
	
	// bus signals
	lomo_player_signals[EOS] =
		g_signal_new ("eos",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, eos),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__VOID,
			    G_TYPE_NONE,
			    0);
	lomo_player_signals[ERROR] =
		g_signal_new ("error",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, error),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_POINTER,
			    G_TYPE_NONE,
			    2,
			    G_TYPE_POINTER,
				G_TYPE_POINTER);
	lomo_player_signals[TAG] =
		g_signal_new ("tag",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, tag),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_POINTER,
				G_TYPE_INT);
	lomo_player_signals[ALL_TAGS] =
		g_signal_new ("all-tags",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, all_tags),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__POINTER,
			    G_TYPE_NONE,
			    1,
				G_TYPE_POINTER);

	g_object_class_install_property(object_class, LOMO_PLAYER_PROPERTY_AUTO_PARSE,
		g_param_spec_boolean("auto-parse", "auto-parse", "Auto parse added streams",
		TRUE, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_type_class_add_private (klass, sizeof (LomoPlayerPrivate));
}

static void lomo_player_init (LomoPlayer *self)
{
	LomoPlayerVTable vtable = {
		create_pipeline,
		destroy_pipeline,

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

	self->priv = GET_PRIVATE(self);
	self->priv->vtable  = vtable;
	self->priv->options = g_hash_table_new(g_str_hash, g_str_equal);
	self->priv->volume  = 50;
	self->priv->mute    = FALSE;
	self->priv->pl      = lomo_playlist_new();
	self->priv->meta    = lomo_metadata_parser_new();
	self->priv->queue   = g_queue_new();
	g_signal_connect(self->priv->meta, "tag", (GCallback) tag_cb, self);
	g_signal_connect(self->priv->meta, "all-tags", (GCallback) all_tags_cb, self);
}

// --
// Core functions
// --
LomoPlayer*
lomo_player_new(const gchar *option_name, ...)
{
	LomoPlayer *self = g_object_new (LOMO_TYPE_PLAYER, NULL);
	va_list args;

	va_start(args, option_name);

	while (option_name != NULL)
	{
		gchar *value = va_arg(args, gchar*);
		g_hash_table_replace(self->priv->options, (gpointer) option_name, (gpointer) value);

		option_name = va_arg(args, gchar*);
	}
	va_end(args);

	if (g_hash_table_lookup(self->priv->options, "audio-output") == NULL)
	{
		g_warning("audio-output option is mandatory while using lomo_player_new_with_opts\n");
		g_object_unref(self);
		return NULL;
	}

	return self;
}

gboolean
lomo_player_get_auto_parse(LomoPlayer *self)
{
	return GET_PRIVATE(self)->auto_parse;
}

void
lomo_player_set_auto_parse(LomoPlayer *self, gboolean auto_parse)
{
	GET_PRIVATE(self)->auto_parse = auto_parse;
}

gboolean
lomo_player_run_hooks(LomoPlayer *self, LomoPlayerHookType type, gpointer ret, ...)
{
	GList *iter = self->priv->hooks;
	GList *iter2 = self->priv->hooks_data;

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
		event.tag    = va_arg(args, LomoTag);
		break;

	case LOMO_PLAYER_HOOK_ALL_TAGS:
		event.stream = va_arg(args, LomoStream*);
		break;

	case LOMO_PLAYER_HOOK_ERROR:
		event.error = va_arg(args, GError*);
		break;

	// Use #if 0 to catch unhandled hooks at compile time
	#if 0
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

// --
// Quick play functions, simple shortcuts.
// --
gboolean
lomo_player_play_uri(LomoPlayer *self, gchar *uri, GError **error)
{
	g_return_val_if_fail(uri, FALSE);
	return lomo_player_play_stream(self, lomo_stream_new(uri), error);
}

gboolean
lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error)
{
	g_return_val_if_fail(stream, FALSE);

	lomo_player_clear(self);
	lomo_player_append(self, stream);

	return (lomo_player_create_pipeline(self, stream, error) && lomo_player_play(self, error));
}

gboolean
lomo_player_create_pipeline(LomoPlayer *self, LomoStream *stream, GError **error)
{
	check_method_or_return_val(self, create_pipeline,  FALSE, error);

	if (!lomo_player_destroy_pipeline(self, error))
		return FALSE;
	self->priv->pipeline = NULL;

	if (stream == NULL)
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_CREATE_PIPELINE,
			N_("Cannot create pipeline for NULL stream"));
		return FALSE;
	}

	self->priv->pipeline = self->priv->vtable.create_pipeline(lomo_stream_get_tag(stream, LOMO_TAG_URI), self->priv->options);
	if (self->priv->pipeline == NULL)
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_CREATE_PIPELINE,
			N_("Cannot create pipeline for stream %s"), (gchar*) lomo_stream_get_tag(stream, LOMO_TAG_URI));
		return FALSE;
	}
	lomo_player_set_volume(self, -1);               // Restore pipeline volume
	lomo_player_set_mute  (self, self->priv->mute); // Restore pipeline mute 
	gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(self->priv->pipeline)), (GstBusFunc) bus_watcher, self);

	return TRUE;
}

gboolean
lomo_player_destroy_pipeline(LomoPlayer *self, GError **error)
{
	check_method_or_return_val(self, destroy_pipeline, FALSE, error);

	if (self->priv->pipeline == NULL)
		return TRUE;

	if (!lomo_player_set_state(self, LOMO_STATE_STOP, error))
		return FALSE;

	self->priv->vtable.destroy_pipeline(self->priv->pipeline);
	self->priv->pipeline = NULL;
	return TRUE;
}

void
lomo_player_hook_add(LomoPlayer *self, LomoPlayerHook func, gpointer data)
{
	GET_PRIVATE(self)->hooks      = g_list_prepend(GET_PRIVATE(self)->hooks, func);
	GET_PRIVATE(self)->hooks_data = g_list_prepend(GET_PRIVATE(self)->hooks_data, data);
}

void
lomo_player_hook_remove(LomoPlayer *self, LomoPlayerHook func)
{
	LomoPlayerPrivate *priv = GET_PRIVATE(self);

	gint index = g_list_index(priv->hooks, func);
	g_return_if_fail(index >= 0);

	GList *p = g_list_nth(priv->hooks, index);
	priv->hooks = g_list_delete_link(priv->hooks, p);

	p = g_list_nth(priv->hooks_data, index);
	priv->hooks_data = g_list_delete_link(priv->hooks_data, p);
}

LomoStream*
lomo_player_get_stream(LomoPlayer *self)
{
	// Hold this for a while to watch the DPP bug
	if (lomo_playlist_nth_stream(self->priv->pl, lomo_playlist_get_current(self->priv->pl)) != self->priv->stream)
		g_printf("[liblomo (%s:%d)] DPP (desyncronized playlist and player) bug found\n", __FILE__, __LINE__);
	return self->priv->stream;
}

// --
// get/set state (using vfuncs)
// --
LomoStateChangeReturn
lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error)
{
	check_method_or_return_val(self, set_state, LOMO_STATE_CHANGE_FAILURE, error);

	if (self->priv->pipeline == NULL)
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_MISSING_PIPELINE,
			N_("Cannot set state: missing pipeline"));
		return LOMO_STATE_CHANGE_FAILURE;
	}

	GstState gst_state;
	if (!lomo_state_to_gst(state, &gst_state))
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_UNKNOW_STATE,
			"Unknow state '%d'", state);
		return LOMO_STATE_CHANGE_FAILURE;
	}


	GstStateChangeReturn ret = self->priv->vtable.set_state(self->priv->pipeline, gst_state);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_SET_STATE,
			N_("Error setting state"));
		return LOMO_STATE_CHANGE_FAILURE;
	}

	return LOMO_STATE_CHANGE_SUCCESS; // Or async, or preroll...
}

LomoState lomo_player_get_state(LomoPlayer *self)
{
	check_method_or_return_val(self, get_state, LOMO_STATE_INVALID, NULL);

	if (!self->priv->pipeline || !self->priv->stream)
		return LOMO_STATE_STOP;
	
	GstState gst_state = self->priv->vtable.get_state(self->priv->pipeline);

	LomoState state;
	if (!lomo_state_from_gst(gst_state, &state))
		return LOMO_STATE_INVALID;

	return state;
}

// --
// get/set position (using vfuncs)
// get lenght (using vfuncs)
// --
gint64 lomo_player_tell(LomoPlayer *self, LomoFormat format)
{
	check_method_or_return_val(self, get_position, LOMO_STATE_INVALID, NULL);

	if (!self->priv->pipeline || !self->priv->stream)
		return -1;

	GstFormat gst_format;
	if (!lomo_format_to_gst(format, &gst_format))
		return -1;

	gint64 ret;
	if (!self->priv->vtable.get_position(self->priv->pipeline, &gst_format, &ret))
		return -1;

	return ret;
}

gboolean
lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val)
{
	check_method_or_return_val(self, set_position, FALSE, NULL);

	if ((self->priv->pipeline == NULL) || (self->priv->stream == NULL))
	{
		g_warning(N_("Player not seekable"));
		return FALSE;
	}

	// Incorrect format
	GstFormat gst_format;
	if (!lomo_format_to_gst(format, &gst_format))
	{
		g_warning(N_("Invalid format"));
		return FALSE;
	}

	gint64 old_pos = lomo_player_tell(self, format);
	if (old_pos == -1)
		return FALSE;

	// Call hooks
	gboolean ret = FALSE;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_SEEK, &ret, old_pos, val))
		return ret;

	// Exec action
	ret = self->priv->vtable.set_position(self->priv->pipeline, gst_format, val);
	if (ret)
		g_signal_emit(G_OBJECT(self), lomo_player_signals[SEEK], 0, old_pos, val);
	else
		g_warning(N_("Error seeking"));

	return ret;
}

gint64
lomo_player_length(LomoPlayer *self, LomoFormat format)
{
	check_method_or_return_val(self, get_length, -1, NULL);

	if ((self->priv->pipeline == NULL) || (self->priv->stream == NULL))
	{
		// g_warning(N_("Cannot get length of NULL stream"));
		// XXX On -1 set seek controls insensitive
		return -1;
	}

	// Format
	GstFormat gst_format;
	if (!lomo_format_to_gst(format, &gst_format)) 
	{
		g_warning("Invalid format");
		return -1;
	}

	// Length
	gint64 ret;
	if (!self->priv->vtable.get_length(self->priv->pipeline, &gst_format, &ret))
	{
		// g_warning(N_("Error getting length"));
		// Maybe pipeline is in his first microsecs of life with no ide of
		// length, dont warn
		return -1;
	}

	return ret;
}

// --
// get/set volume (uses vfuncs)
//--
gboolean lomo_player_set_volume(LomoPlayer *self, gint val)
{
	check_method_or_return_val(self, set_volume, FALSE, NULL);
	
	if (val == -1)
		val = self->priv->volume;
	val = CLAMP(val, 0, 100);

	if (self->priv->pipeline == NULL)
	{
		// g_warning(N_("Cannot set volume on a NULL pipeline"));
		// return FALSE;
		self->priv->volume = val;
		return TRUE;
	}

	// Call hooks
	gboolean ret = FALSE;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_VOLUME, &ret, val))
		return ret;

	// Exec action
	if (!self->priv->vtable.set_volume(self->priv->pipeline, val))
	{
		g_warning(N_("Error setting volume"));
		return FALSE;
	}

	self->priv->volume = val;
	g_signal_emit(self, lomo_player_signals[VOLUME], 0, val);
	return TRUE;
}

gint lomo_player_get_volume(LomoPlayer *self)
{
	if (self->priv->pipeline == NULL)
	{
		// g_warning(N_("Cannot get volume form a NULL pipeline"));
		// return -1;
		return self->priv->volume;
	}

	if (self->priv->vtable.get_volume == NULL)
		return self->priv->volume;
	
	gint ret = self->priv->vtable.get_volume(self->priv->pipeline);
	if (ret == -1)
		g_warning(N_("Error getting volume"));
	return ret;
}

// --
// get/set mute (uses vfuncs)
// --
gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute)
{
	if ((self->priv->vtable.set_mute == NULL) && (self->priv->vtable.set_volume == NULL))
	{
		g_warning(N_("Missing set_mute and set_volume methods, cannot mute"));
		return FALSE;
	}

	// Run hooks
	gboolean ret = FALSE;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_MUTE, &ret, mute))
		return ret;

	if (self->priv->pipeline == NULL)
	{
		self->priv->mute = mute;
		return TRUE;
	}

	// Exec action
	if (self->priv->pipeline == NULL)
	{
		self->priv->mute = mute;
		ret = TRUE;
	}
	else
	{
		if (self->priv->vtable.set_mute)
			ret = self->priv->vtable.set_mute(self->priv->pipeline, mute);
		else
			ret = self->priv->vtable.set_volume(self->priv->pipeline, mute ? 0 : self->priv->volume);
	}

	if (!ret)
	{
		g_warning(N_("Error setting mute"));
		return ret;
	}

	self->priv->mute = mute;
	g_signal_emit(self, lomo_player_signals[MUTE], 0 , mute);

	return ret;
}

gboolean lomo_player_get_mute(LomoPlayer *self)
{
	if (self->priv->vtable.get_mute)
		return self->priv->vtable.get_mute(self->priv->pipeline);
	else
		return self->priv->mute;
}

// --
// Playlist functions
// --
void
lomo_player_insert(LomoPlayer *self, LomoStream *stream, gint pos)
{
	GList *tmp = NULL;

	tmp = g_list_prepend(tmp, stream);
	lomo_player_insert_multi(self, tmp, pos);
	g_list_free(tmp);
}

void
lomo_player_insert_uri_multi(LomoPlayer *self, GList *uris, gint pos)
{
	GList *l, *streams = NULL;
	LomoStream *stream = NULL;

	l = uris;
	while (l) {
		if ((stream = lomo_stream_new((gchar *) l->data)) != NULL)
			streams = g_list_prepend(streams, stream);
		l = l->next;
	}
	streams = g_list_reverse(streams);

	lomo_player_insert_multi(self, streams, pos);
	g_list_free(streams);
}

void
lomo_player_insert_uri_strv(LomoPlayer *self, gchar **uris, gint pos)
{
	GList *l = NULL;
	gint i;
	gchar *tmp;

	if (uris == NULL)
		return; 
	
	for (i = 0; uris[i] != NULL; i++)
	{
		if ((tmp = g_uri_parse_scheme(uris[i])) == NULL)
		{
			if ((tmp = g_filename_to_uri(uris[i], NULL, NULL)) != NULL)
				l = g_list_prepend(l, tmp);
		}
		else
		{
			g_free(tmp);
			l = g_list_prepend(l, uris[i]);
		}
	}

	l = g_list_reverse(l);
	lomo_player_insert_uri_multi(self, l, pos);
	g_list_free(l);
}

// XXX: Add hook calls
void
lomo_player_insert_multi(LomoPlayer *self, GList *streams, gint pos)
{
	GList *l;
	LomoStream *stream = NULL;
	gboolean emit_change;

	if (streams == NULL)
		return;

	// We should emit change if player was empty before add those streams
	if (lomo_player_get_total(self) == 0)
		emit_change = TRUE;
	else
		emit_change = FALSE;

	// Add streams to playlist
	if ((pos <= 0) || (pos >  lomo_player_get_total(self)))
		pos = lomo_player_get_total(self);
	// lomo_playlist_insert_multi(self->priv->pl, streams, pos);

	// For each one parse metadata and emit signals 
	l = streams;
	while (l)
	{
		stream = (LomoStream *) l->data;

		// Run hooks
		if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_INSERT, NULL, stream, pos))
		{
			l = l->next;
			continue;
		}

		// Exec action
		lomo_playlist_insert(self->priv->pl, stream, pos);
		if (self->priv->auto_parse)
			lomo_metadata_parser_parse(self->priv->meta, stream, LOMO_METADATA_PARSER_PRIO_DEFAULT);
		g_signal_emit(G_OBJECT(self), lomo_player_signals[INSERT], 0, stream, pos);
	
		// Emit change if its first stream and hooks dont catch change
		if (emit_change && !lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_CHANGE, NULL, -1, 0))
		{
			g_signal_emit(G_OBJECT(self), lomo_player_signals[CHANGE], 0, -1, 0);
			GError *err = NULL;
			if (!lomo_player_create_pipeline(self, stream, &err))
			{
				g_warning(N_("Error creating pipeline: %s"), err->message);
				g_error_free(err);
			}
			else
				emit_change = FALSE;
		}

		pos++;
		
		l = l->next;
	}
}

gboolean lomo_player_del(LomoPlayer *self, gint pos)
{
	gint curr, next;

	if (lomo_player_get_total(self) <= pos )
		return FALSE;

	LomoStream *stream = lomo_player_nth_stream(self, pos);

	// Call hooks
	gboolean ret = FALSE;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_REMOVE, &ret, stream, pos))
		return ret;

	// Exec action
	g_object_ref(stream);
	curr = lomo_player_get_current(self);
	if (curr != pos)
	{
		// No problem, delete 
		lomo_playlist_del(self->priv->pl, pos);
	}

	else
	{
		// Try to go next 
		next = lomo_player_get_next(self);
		if ((next == curr) || (next == -1))
		{
			// mmm, only one stream, go stop
			lomo_player_stop(self, NULL);
			lomo_playlist_del(self->priv->pl, pos);
		}
		else
		{
			/* Delete and go next */
			lomo_player_go_next(self, NULL);
			lomo_playlist_del(self->priv->pl, pos);
		}
	}
	g_signal_emit(G_OBJECT(self), lomo_player_signals[REMOVE], 0, stream, pos);
	g_object_unref(stream);

	return TRUE;
}

gint lomo_player_queue(LomoPlayer *self, gint pos)
{
	g_return_val_if_fail(pos >= 0, -1);

	LomoStream *stream = lomo_playlist_nth_stream(self->priv->pl, pos);
	g_return_val_if_fail(stream != NULL, -1);

	// Run hooks
	gint ret = -1;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_QUEUE, &ret, stream, g_queue_get_length(self->priv->queue)))
		return ret;

	// Exec action
	g_queue_push_tail(self->priv->queue, stream);
	gint queue_pos = g_queue_get_length(self->priv->queue) - 1;

	g_signal_emit(G_OBJECT(self), lomo_player_signals[QUEUE], 0, stream, queue_pos);
	return queue_pos;
}

gboolean lomo_player_dequeue(LomoPlayer *self, gint queue_pos)
{
	LomoStream *stream = g_queue_peek_nth(self->priv->queue, queue_pos);
	g_return_val_if_fail(stream != NULL, FALSE);

	// Run hooks
	gboolean ret = FALSE;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_DEQUEUE, &ret, stream, queue_pos))
		return ret;

	// Exec action
	if (g_queue_pop_nth(self->priv->queue, queue_pos) == NULL)
		return FALSE;

	g_signal_emit(G_OBJECT(self), lomo_player_signals[DEQUEUE], 0, stream, queue_pos);
	return TRUE;
}

gint lomo_player_queue_index(LomoPlayer *self, LomoStream *stream)
{
	return g_queue_index(self->priv->queue, stream);
}

LomoStream *lomo_player_queue_nth(LomoPlayer *self, guint queue_pos)
{
	return g_queue_peek_nth(self->priv->queue, queue_pos);
}

void lomo_player_queue_clear(LomoPlayer *self)
{
	// Run hooks
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_QUEUE_CLEAR, NULL))
		return;

	// Exec action
	g_queue_clear(self->priv->queue);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[QUEUE_CLEAR], 0);
}

GList *lomo_player_get_playlist(LomoPlayer *self)
{
	return lomo_playlist_get_playlist(self->priv->pl);
}

LomoStream *lomo_player_nth_stream(LomoPlayer *self, gint pos)
{
	return lomo_playlist_nth_stream(self->priv->pl, pos);
}

gint
lomo_player_index(LomoPlayer *self, LomoStream *stream)
{
	return lomo_playlist_index(self->priv->pl, stream);
	return lomo_playlist_index(self->priv->pl, stream);
}

gint lomo_player_get_prev(LomoPlayer *self)
{
	return lomo_playlist_get_prev(self->priv->pl);
}

gint lomo_player_get_next(LomoPlayer *self)
{
	LomoStream *stream = g_queue_peek_head(self->priv->queue);
	if (stream)
		return lomo_playlist_index(self->priv->pl, stream);
	else
		return lomo_playlist_get_next(self->priv->pl);
}

gboolean lomo_player_go_nth(LomoPlayer *self, gint pos, GError **error)
{
	const LomoStream *stream;
	LomoState state;
	gint prev = -1;

	// Cannot go to that position
	stream = lomo_playlist_nth_stream(self->priv->pl, pos);
	if (stream == NULL)
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_NO_STREAM, "No stream at position %d", pos);
		return FALSE;
	}

	// Check if stream is in queue and dequeue it
	gint queue_idx = g_queue_index(self->priv->queue, stream);
	if ((queue_idx >= 0) && !lomo_player_dequeue(self, queue_idx))
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_CANNOT_DEQUEUE, N_("Cannot change stream, it's in queue but can't dequeue it"));
		return FALSE;
	}

	// Get state for later restore it
	state = lomo_player_get_state(self);
	if (state == LOMO_STATE_INVALID)
		state = LOMO_STATE_STOP;

	// Emit prechange for cleanup (dont call hook because pre-change signal its
	// deprecated and overlaps with change hook)
	g_signal_emit(G_OBJECT(self), lomo_player_signals[PRE_CHANGE], 0);

	prev = lomo_player_get_current(self);

	// Call hook
	gboolean ret = FALSE;
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_CHANGE, &ret, prev, pos))
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_HOOK_BLOCK, N_("Action blocked by hook"));
		return ret;
	}

	// Exec action
	if (!lomo_player_stop(self, error))
		return FALSE;

	lomo_playlist_go_nth(self->priv->pl, pos);
	self->priv->stream = (LomoStream *) stream;

	if (!lomo_player_create_pipeline(self, (LomoStream *) stream, error))
		return FALSE;

	g_signal_emit(G_OBJECT(self), lomo_player_signals[CHANGE], 0, prev, pos);

	// Restore state
	if (lomo_player_set_state(self, state, NULL) == LOMO_STATE_CHANGE_FAILURE) 
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE, "Error while changing state");
		return FALSE;
	}

	return TRUE;
}

gint lomo_player_get_current(LomoPlayer *self)
{
	return lomo_playlist_get_current(self->priv->pl);
}

guint lomo_player_get_total(LomoPlayer *self)
{
	return lomo_playlist_get_total(self->priv->pl);
}

void lomo_player_clear(LomoPlayer *self)
{
	// Only clear if needed
	if (lomo_player_get_playlist(self) == NULL)
		return;

	// Run hook
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_CLEAR, NULL))
		return;

	// Exec action
	lomo_player_stop(self, NULL);
	lomo_playlist_clear(self->priv->pl);
	lomo_metadata_parser_clear(self->priv->meta);
	lomo_player_destroy_pipeline(self, NULL);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[CLEAR], 0);
}

void lomo_player_set_repeat(LomoPlayer *self, gboolean val)
{
	// Run hook
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_REPEAT, NULL, val))
		return;

	// Exec action
	lomo_playlist_set_repeat(self->priv->pl, val);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[REPEAT], 0, val);
}

gboolean lomo_player_get_repeat(LomoPlayer *self)
{
	return lomo_playlist_get_repeat(self->priv->pl);
}

void lomo_player_set_random(LomoPlayer *self, gboolean val)
{
	// Run hook
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_RANDOM, NULL, val))
		return;

	// Exec action
	lomo_playlist_set_random(self->priv->pl, val);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[RANDOM], 0, val);
}

gboolean lomo_player_get_random(LomoPlayer *self)
{
	return lomo_playlist_get_random(self->priv->pl);
}

void lomo_player_randomize(LomoPlayer *self)
{
	lomo_playlist_randomize(self->priv->pl);
}

void
lomo_player_print_pl(LomoPlayer *self)
{
	lomo_playlist_print(self->priv->pl);
}

void
lomo_player_print_random_pl(LomoPlayer *self)
{
	lomo_playlist_print_random(self->priv->pl);
}

// --
// Watchers and callbacks
// --
static void
tag_cb(LomoMetadataParser *parser, LomoStream *stream, LomoTag tag, LomoPlayer *self)
{
	// Run hook
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_TAG, NULL, stream, tag))
		return;

	// Exec signal
	g_signal_emit(self, lomo_player_signals[TAG], 0, stream, tag);
}

static void
all_tags_cb(LomoMetadataParser *parser, LomoStream *stream, LomoPlayer *self)
{
	// Run hook
	if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_ALL_TAGS, NULL, stream))
		return;

	// Exec action
	g_signal_emit(self, lomo_player_signals[ALL_TAGS], 0, stream);
}

static gboolean
bus_watcher(GstBus *bus, GstMessage *message, LomoPlayer *self)
{
	GError *err = NULL;
	gchar *debug = NULL;
	LomoStream *stream = NULL;

	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(message, &err, &debug);
			stream = lomo_player_get_stream(self);

			// Call hook
			if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_ERROR, NULL, stream, err))
			{
				g_error_free(err);
				g_free(debug);
				break;
			}

			// Exec action
			if (stream != NULL)
				lomo_stream_set_failed_flag(stream, TRUE);

			g_signal_emit(G_OBJECT(self), lomo_player_signals[ERROR], 0, stream, err);
			g_error_free(err);
			g_free(debug);
			break;

		case GST_MESSAGE_EOS:
			// g_printf("===> %"G_GINT64_FORMAT" (%"G_GINT64_FORMAT" secs)\n", lomo_player_tell_time(self), lomo_nanosecs_to_secs(lomo_player_tell_time(self)));
			if (lomo_player_run_hooks(self, LOMO_PLAYER_HOOK_EOS, NULL))
				break;

			g_signal_emit(G_OBJECT(self), lomo_player_signals[EOS], 0);
			break;

		case GST_MESSAGE_STATE_CHANGED:
		{
			static guint last_signal;
			guint signal;
			LomoPlayerHookType hook;

			GstState oldstate, newstate, pending;
			gst_message_parse_state_changed(message, &oldstate, &newstate, &pending);
			/*
			g_printf("Got state change from bus: old=%s, new=%s, pending=%s\n",
				gst_state_to_str(oldstate),
				gst_state_to_str(newstate),
				gst_state_to_str(pending));
			*/
			if (pending != GST_STATE_VOID_PENDING)
				break;

			switch (newstate)
			{
			case GST_STATE_NULL:
			case GST_STATE_READY:
				signal = lomo_player_signals[STOP];
				hook   = LOMO_PLAYER_HOOK_STOP;
				break;
			case GST_STATE_PAUSED:
				// Ignore pause events before 50 miliseconds, gstreamer pauses
				// pipeline after a first play event, returning to play state
				// inmediatly. 
				if ((lomo_player_tell_time(self) / 1000000) > 50)
				{
					signal = lomo_player_signals[PAUSE];
					hook   = LOMO_PLAYER_HOOK_PAUSE;
				}
				else
					return TRUE;
				break;
			case GST_STATE_PLAYING:
				signal = lomo_player_signals[PLAY];
				hook   = LOMO_PLAYER_HOOK_PLAY;
				break;
			default:
				g_printf("ERROR: Unknow state transition: %s\n", gst_state_to_str(newstate));
				return TRUE;
			}

			// Filter repeated events
			if (signal != last_signal)
			{
				if (!lomo_player_run_hooks(self, hook, NULL))
				{
					// g_printf("Emit signal %s\n", gst_state_to_str(newstate));
					g_signal_emit(G_OBJECT(self), signal, 0);
					last_signal = signal;
				}
			}
			break;
		}

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
		case GST_MESSAGE_ELEMENT:
		case GST_MESSAGE_SEGMENT_START:
		case GST_MESSAGE_SEGMENT_DONE:
		case GST_MESSAGE_DURATION:
			// g_printf("Bus got something like... '%s'\n", gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
			break;
			
		default:
			break;
	}

	return TRUE;
}

// --
// Default functions for LomoPlayerVTable
// --
static GstElement*
create_pipeline(const gchar *uri, GHashTable *opts)
{
	GstElement *ret, *audio_sink;
	const gchar *audio_sink_str;
	
	if ((ret = gst_element_factory_make("playbin", "playbin")) == NULL)
		return NULL;

	audio_sink_str = (gchar *) g_hash_table_lookup(opts, (gpointer) "audio-output");
	if (audio_sink_str == NULL)
		audio_sink_str = "autoaudiosink";
	
	audio_sink = gst_element_factory_make(audio_sink_str, "audio-sink");
	if (audio_sink == NULL)
	{
		g_object_unref(ret);
		return NULL;
	}

	g_object_set(G_OBJECT(ret),
		"audio-sink", audio_sink,
		"uri", uri,
		NULL);

	gst_element_set_state(ret, GST_STATE_READY); 

	return ret;
}

static void
destroy_pipeline(GstElement *pipeline)
{
	gst_element_set_state(pipeline, GST_STATE_NULL);
	g_object_unref(G_OBJECT(pipeline));
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

