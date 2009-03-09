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

#ifdef LOMO_DEBUG
#define TRACE g_printf("[LomoPlayer Trace] %s %d\n", __FUNCTION__, __LINE__);
#else
#define TRACE ((void)(0));
#endif

#define lomo_player_set_error(error,code,...) \
	do { \
		if (error) \
			*error = g_error_new(lomo_quark(), code,__VA_ARGS__); \
	} while(0)

G_DEFINE_TYPE(LomoPlayer, lomo_player, G_TYPE_OBJECT)

enum {
	LOMO_PLAYER_ERROR_NO_PIPELINE = 1,
	LOMO_PLAYER_ERROR_NO_STREAM,
	LOMO_PLAYER_ERROR_UNKNOW_STATE,
	LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE,
	LOMO_PLAYER_ERROR_STATE_FAILURE,
	LOMO_PLAYER_ERROR_GENERIC,

	// New core
	LOMO_PLAYER_ERROR_NO_VFUNC,
	LOMO_PLAYER_ERROR_GET_STATE,
	LOMO_PLAYER_ERROR_INVALID_VALUE
};

struct _LomoPlayerPrivate {
	LomoPlayerVTable  vtable;
	GHashTable       *options;

	GstElement         *pipeline;
	LomoPlaylist       *pl;
	LomoMetadataParser *meta;
	LomoStream         *stream;

	gint          volume;
	gboolean      mute;
};

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_PLAYER, LomoPlayerPrivate))

enum {
	PLAY,
	PAUSE,
	STOP,
	SEEK,
	VOLUME,
	MUTE,
	ADD,
	DEL,
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

// --
// LomoPlayer
// --
static GQuark
lomo_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("lomo-quark");
	return ret;
}

static void
lomo_player_dispose(GObject *object)
{ TRACE
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
{ TRACE
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

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
	lomo_player_signals[ADD] =
		g_signal_new ("add",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, add),
			    NULL, NULL,
			    lomo_marshal_VOID__POINTER_INT,
			    G_TYPE_NONE,
			    2,
				G_TYPE_POINTER,
				G_TYPE_INT);
	lomo_player_signals[DEL] =
		g_signal_new ("del",
			    G_OBJECT_CLASS_TYPE (object_class),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (LomoPlayerClass, del),
			    NULL, NULL,
			    g_cclosure_marshal_VOID__INT,
			    G_TYPE_NONE,
			    1,
				G_TYPE_INT);
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

	g_type_class_add_private (klass, sizeof (LomoPlayerPrivate));
}

static void lomo_player_init (LomoPlayer *self)
{ TRACE
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

	// Transitional code
	if (g_hash_table_lookup(self->priv->options, "audio-output") == NULL)
	{
		g_warning("audio-output option is mandatory while using lomo_player_new_with_opts\n");
		g_object_unref(self);
		return NULL;
	}

	return self;
}

// --
// Quick play functions, simple shortcuts.
// --
gboolean
lomo_player_play_uri(LomoPlayer *self, gchar *uri, GError **error)
{ TRACE
	g_return_val_if_fail(uri, FALSE);
	return lomo_player_play_stream(self, lomo_stream_new(uri), error);
}

gboolean
lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error)
{ TRACE
	g_return_val_if_fail(stream, FALSE);

	lomo_player_clear(self);
	lomo_player_add(self, stream);

	return (lomo_player_reset(self, error) && lomo_player_play(self, error));
}

gboolean lomo_player_reset(LomoPlayer *self, GError **error)
{ TRACE
	g_return_val_if_fail(self->priv->vtable.create_pipeline,  FALSE);
	g_return_val_if_fail(self->priv->vtable.destroy_pipeline, FALSE);

	// Destroy pipeline
	if (self->priv->pipeline)
	{
		lomo_player_stop(self, NULL);
		self->priv->vtable.destroy_pipeline(self->priv->pipeline);
		self->priv->pipeline = NULL;
	}

	// Sync current stream
	gint current = lomo_playlist_get_current(self->priv->pl);
	if (current == -1)
	{
		self->priv->stream = NULL;
		return TRUE;
	}
	else
		self->priv->stream = lomo_playlist_get_nth(self->priv->pl, current);

	// Sometimes stream tag's hasnt been parsed, in this case we move stream to
	// inmediate queue on LomoMetadataParser object to get them ASAP
	if (!lomo_stream_get_all_tags_flag(self->priv->stream))
		lomo_metadata_parser_parse(self->priv->meta, self->priv->stream, LOMO_METADATA_PARSER_PRIO_INMEDIATE);

	// Now, create pipeline
	if (!(self->priv->pipeline = self->priv->vtable.create_pipeline(lomo_stream_get_tag(self->priv->stream, LOMO_TAG_URI), self->priv->options)))
		return FALSE;
	
	// Attach a bus watch
	gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(self->priv->pipeline)), (GstBusFunc) bus_watcher, self);

	// Setup pipeline
	lomo_player_set_volume(self, self->priv->volume);
	lomo_player_set_mute  (self, self->priv->mute);

	return TRUE;
}

LomoStream*
lomo_player_get_stream(LomoPlayer *self)
{ TRACE
	// Hold this for a while to watch the DPP bug
	if (lomo_playlist_get_nth(self->priv->pl, lomo_playlist_get_current(self->priv->pl)) != self->priv->stream)
		g_printf("[liblomo (%s:%d)] DPP (desyncronized playlist and player) bug found\n", __FILE__, __LINE__);
	return self->priv->stream;
}

// --
// get/set state (using vfuncs)
// --
LomoStateChangeReturn
lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error)
{ TRACE
	g_return_val_if_fail(self->priv->vtable.set_state, LOMO_STATE_CHANGE_FAILURE);
	g_return_val_if_fail(self->priv->pipeline, LOMO_STATE_CHANGE_FAILURE);
	g_return_val_if_fail(self->priv->stream, LOMO_STATE_CHANGE_FAILURE);

	GstState gst_state;
	if (!lomo_state_to_gst(state, &gst_state))
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_UNKNOW_STATE, "Unknow state '%d'", state);
		return LOMO_STATE_CHANGE_FAILURE;
	}

	if ((gst_state != GST_STATE_PAUSED) && (gst_state != GST_STATE_PLAYING) && (gst_state != GST_STATE_NULL))
		return LOMO_STATE_CHANGE_FAILURE;
	
	GstStateChangeReturn ret = self->priv->vtable.set_state(self->priv->pipeline, gst_state);
	if (state == LOMO_STATE_STOP)
		lomo_player_reset(self, NULL);

	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		g_set_error(error, lomo_quark(), LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE, "Error while setting state on pipeline");
		return LOMO_STATE_CHANGE_FAILURE;
	}

	return LOMO_STATE_CHANGE_SUCCESS; // Or async, or preroll...
}

LomoState lomo_player_get_state(LomoPlayer *self)
{ TRACE
	g_return_val_if_fail(self->priv->vtable.get_state, LOMO_STATE_INVALID);

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
{ TRACE
	g_return_val_if_fail(self->priv->vtable.get_position, -1);
	g_return_val_if_fail(self->priv->pipeline, -1);
	g_return_val_if_fail(self->priv->stream, -1);

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
{ TRACE
	g_return_val_if_fail(self->priv->vtable.set_position, FALSE);
	g_return_val_if_fail(self->priv->pipeline, FALSE);
	g_return_val_if_fail(self->priv->stream, FALSE);

	// Incorrect format
	GstFormat gst_format;
	if (!lomo_format_to_gst(format, &gst_format))
		return FALSE;

	gint64 old_pos = lomo_player_tell(self, format);
	if (old_pos == -1)
		return FALSE;
	
	gboolean ret = self->priv->vtable.set_position(self->priv->pipeline, gst_format, val);
	if (ret)
		g_signal_emit(G_OBJECT(self), lomo_player_signals[SEEK], 0, old_pos, val);
	return ret;
}

gint64
lomo_player_length(LomoPlayer *self, LomoFormat format)
{ TRACE
	g_return_val_if_fail(self->priv->vtable.get_length != NULL, -1);
	g_return_val_if_fail(self->priv->stream != NULL, -1);

	// Format
	GstFormat gst_format;
	if (!lomo_format_to_gst(format, &gst_format)) 
		return -1;

	// Length
	gint64 ret;
	if (!self->priv->vtable.get_length(self->priv->pipeline, &gst_format, &ret))
		return -1;

	return ret;
}

// --
// get/set volume (uses vfuncs)
//--
gboolean lomo_player_set_volume(LomoPlayer *self, gint val)
{ TRACE
	// Check vtable
	g_return_val_if_fail(self->priv->vtable.set_volume != NULL, FALSE);

	if (self->priv->pipeline && !self->priv->vtable.set_volume(self->priv->pipeline, CLAMP(val, 0, 100)))
		return FALSE;

	self->priv->volume = val;
	g_signal_emit(self, lomo_player_signals[VOLUME], 0, val);
	return TRUE;
}

gint lomo_player_get_volume(LomoPlayer *self)
{ TRACE
	// Call vfunc if needed
	if (self->priv->vtable.get_volume)
	{
		g_return_val_if_fail(self->priv->pipeline, -1);
		return self->priv->vtable.get_volume(self->priv->pipeline);
	}
	else
		return self->priv->volume;
}

// --
// get/set mute (uses vfuncs)
// --
gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute)
{ TRACE
	// Check vtable
	g_return_val_if_fail((self->priv->vtable.set_mute != NULL) || (self->priv->vtable.set_volume != NULL), FALSE);
	// Need pipeline, not mandatory
	g_return_val_if_fail(self->priv->pipeline != NULL, FALSE);

	gint vol;
	if (mute)
		vol = 0;
	else
		vol = self->priv->volume;

	gboolean ret;
	if (self->priv->vtable.set_mute)
		ret = self->priv->vtable.set_mute(self->priv->pipeline, mute);
	else
		ret = self->priv->vtable.set_volume(self->priv->pipeline, vol);

	if (ret)
	{
		self->priv->mute = mute;
		g_signal_emit(self, lomo_player_signals[MUTE], 0 , mute);
	}

	return ret;
}

gboolean lomo_player_get_mute(LomoPlayer *self)
{ TRACE
	if (self->priv->vtable.get_mute)
		return self->priv->vtable.get_mute(self->priv->pipeline);
	else
		return self->priv->mute;
}

/*
 * Playlist functions
 */
gint lomo_player_add_at_pos(LomoPlayer *self, LomoStream *stream, gint pos)
{ TRACE
	GList *tmp = NULL;
	gint ret;

	tmp = g_list_prepend(tmp, stream);
	ret = lomo_player_add_multi_at_pos(self, tmp, pos);
	g_list_free(tmp);

	return ret;
}

gint lomo_player_add_uri_multi_at_pos(LomoPlayer *self, GList *uris, gint pos)
{ TRACE
	GList *l, *streams = NULL;
	LomoStream *stream = NULL;
	gint ret;

	l = uris;
	while (l) {
		if ((stream = lomo_stream_new((gchar *) l->data)) != NULL)
			streams = g_list_prepend(streams, stream);
		l = l->next;
	}
	streams = g_list_reverse(streams);

	ret = lomo_player_add_multi_at_pos(self, streams, pos);
	g_list_free(streams);

	return ret;
}

gint lomo_player_add_uri_strv_at_pos(LomoPlayer *self, gchar **uris, gint pos)
{ TRACE
	GList *l = NULL;
	gint ret, i;
	gchar *tmp;

	if (uris == NULL)
		return 0; 
	
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
	ret = lomo_player_add_uri_multi_at_pos(self, l, pos);
	g_list_free(l);

	return ret;
}

gint lomo_player_add_multi_at_pos(LomoPlayer *self, GList *streams, gint pos)
{ TRACE
	GList *l;
	LomoStream *stream = NULL;
	gint ret, i, emit_change ;

	if (streams == NULL)
		return 0;

	// We should emit change if player was empty before add those streams
	if (lomo_player_get_total(self) == 0)
		emit_change = TRUE;
	else
		emit_change = FALSE;

	// Add streams to playlist
	i = ret = lomo_playlist_add_multi_at_pos(self->priv->pl, streams, pos);

	// For each one parse metadata and emit signals 
	l = streams;
	while (l)
	{
		stream = (LomoStream *) l->data;

		lomo_metadata_parser_parse(self->priv->meta, stream, LOMO_METADATA_PARSER_PRIO_DEFAULT);
		g_signal_emit(G_OBJECT(self), lomo_player_signals[ADD], 0, stream, i);
	
		// Emit change if its first stream
		if (emit_change)
		{
			g_signal_emit(G_OBJECT(self), lomo_player_signals[CHANGE], 0, -1, 0);
			lomo_player_reset(self, NULL);
			emit_change = FALSE;
		}

		i++;
		
		l = l->next;
	}
	
	return ret;
}


gboolean lomo_player_del(LomoPlayer *self, gint pos)
{ TRACE
	gint curr, next;

	if (lomo_player_get_total(self) <= pos )
		return FALSE;

	curr = lomo_player_get_current(self);
	if (curr != pos)
	{
		// No problem, delete 
		g_signal_emit(G_OBJECT(self), lomo_player_signals[DEL], 0, pos);
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
			g_signal_emit(G_OBJECT(self), lomo_player_signals[DEL], 0, pos);
			lomo_playlist_del(self->priv->pl, pos);
		}
		else
		{
			/* Delete and go next */
			lomo_player_go_next(self, NULL);
			g_signal_emit(G_OBJECT(self), lomo_player_signals[DEL], 0, pos);
			lomo_playlist_del(self->priv->pl, pos);
		}
	}

	return TRUE;
}

GList *lomo_player_get_playlist(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_playlist(self->priv->pl);
}

LomoStream *lomo_player_get_nth(LomoPlayer *self, gint pos)
{ TRACE
	return lomo_playlist_get_nth(self->priv->pl, pos);
}

gint
lomo_player_get_position(LomoPlayer *self, LomoStream *stream)
{ TRACE
	return lomo_playlist_get_position(self->priv->pl, stream);
}

gint lomo_player_get_prev(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_prev(self->priv->pl);
}

gint lomo_player_get_next(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_next(self->priv->pl);
}

gboolean lomo_player_go_nth(LomoPlayer *self, gint pos, GError **error)
{ TRACE
	const LomoStream *stream;
	LomoState state;
	gint prev = -1;

	// Cannot go to that position
	stream = lomo_playlist_get_nth(self->priv->pl, pos);
	if (stream == NULL)
	{
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_NO_STREAM,
			"No stream at position %d", pos);
		return FALSE;
	}

	// Get state for later restore it
	state = lomo_player_get_state(self);
	if (state == LOMO_STATE_INVALID)
		state = LOMO_STATE_STOP;

	// Change
	prev = lomo_player_get_current(self);
	lomo_player_stop(self, NULL);
	lomo_playlist_go_nth(self->priv->pl, pos);
	lomo_player_reset(self, NULL);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[CHANGE], 0, prev, pos);

	// Restore state
	if (lomo_player_set_state(self, state, NULL) == LOMO_STATE_CHANGE_FAILURE) 
	{
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE,
			"Error while changing state");
		return FALSE;
	}

	return TRUE;
}

gint lomo_player_get_current(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_current(self->priv->pl);
}

guint lomo_player_get_total(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_total(self->priv->pl);
}

void lomo_player_clear(LomoPlayer *self)
{ TRACE
	lomo_player_stop(self, NULL);
	lomo_playlist_clear(self->priv->pl);
	lomo_metadata_parser_clear(self->priv->meta);
	lomo_player_reset(self, NULL);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[CLEAR], 0);
}

void lomo_player_set_repeat(LomoPlayer *self, gboolean val)
{ TRACE
	lomo_playlist_set_repeat(self->priv->pl, val);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[REPEAT], 0, val);
}

gboolean lomo_player_get_repeat(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_repeat(self->priv->pl);
}

void lomo_player_set_random(LomoPlayer *self, gboolean val)
{ TRACE
	lomo_playlist_set_random(self->priv->pl, val);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[RANDOM], 0, val);
}

gboolean lomo_player_get_random(LomoPlayer *self)
{ TRACE
	return lomo_playlist_get_random(self->priv->pl);
}

void lomo_player_randomize(LomoPlayer *self)
{ TRACE
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
	g_signal_emit(self, lomo_player_signals[TAG], 0, stream, tag);
}

static void
all_tags_cb(LomoMetadataParser *parser, LomoStream *stream, LomoPlayer *self)
{
	g_signal_emit(self, lomo_player_signals[ALL_TAGS], 0, stream);
}

static gboolean
bus_watcher(GstBus *bus, GstMessage *message, LomoPlayer *self)
{ TRACE
	GError *err = NULL;
	gchar *debug = NULL;
	LomoStream *stream = NULL;

	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(message, &err, &debug);
			if ((stream = lomo_player_get_stream(self)) != NULL)
				lomo_stream_set_failed_flag(stream, TRUE);
			g_signal_emit(G_OBJECT(self), lomo_player_signals[ERROR], 0, stream, err);
			g_error_free(err);
			g_free(debug);
			break;

		case GST_MESSAGE_EOS:
			g_signal_emit(G_OBJECT(self), lomo_player_signals[EOS], 0);
			break;

		case GST_MESSAGE_STATE_CHANGED:
		{
			static guint last_signal;
			guint signal;

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
				break;
			case GST_STATE_PAUSED:
				signal = lomo_player_signals[PAUSE];
				break;
			case GST_STATE_PLAYING:
				signal = lomo_player_signals[PLAY];
				break;
			default:
				g_printf("ERROR: Unknow state transition: %s\n", gst_state_to_str(newstate));
				return TRUE;
			}

			if (signal != last_signal)
			{
				// g_printf("Emit signal %s\n", gst_state_to_str(newstate));
				g_signal_emit(G_OBJECT(self), signal, 0);
				last_signal = signal;
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
// Defaul functions for LomoPlayerVTable
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

