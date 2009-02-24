/*
 * lomo/player.c
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
#include "player.h"
#include "player-priv.h"
#include "pl.h"
#include "meta.h"
#include "lomo-marshal.h"
#include "util.h"
// #include "player-default-vtable.h"

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
	// LomoPlayerVTable  vtable;
	GHashTable       *options;

	LomoStream *stream;

	GstElement *pipeline;

	LomoPlaylist *pl;
	LomoMeta     *meta;
	gint          volume;
	gboolean      mute;
};

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_PLAYER, LomoPlayerPrivate))

static gboolean _lomo_player_bus_watcher (
	GstBus *bus,
	GstMessage *message,
	gpointer data);

static GQuark
lomo_quark(void)
{
	static GQuark ret = 0;
	return ((ret == 0) ? (ret = g_quark_from_static_string("lomo-quark")) : ret);
}

/*
void
lomo_init(gint *argc, gchar **argv[])
{ TRACE
	gst_init(argc, argv);
}
*/

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

	if (self->priv->pipeline) {
		gst_element_set_state(self->priv->pipeline, GST_STATE_NULL);
		g_object_unref(self->priv->pipeline);
	}

	if (self->priv->pl) {
		lomo_playlist_unref(self->priv->pl);
		self->priv->pl = NULL;
	}
	if (self->priv->meta)
	{
		g_object_unref(self->priv->meta);
		self->priv->meta = NULL;
	}
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
	/*
	LomoPlayerVTable vtable = {
		default_create,
		default_destroy,
		NULL,

		default_set_stream,
		NULL,

		default_set_state,
		default_get_state,

		default_query_position,
		default_query_duration,

		default_set_volume,
		default_get_volume,

		NULL,
		NULL
	};
	*/

	self->priv = GET_PRIVATE(self);
	// self->priv->vtable = vtable;
	self->priv->options = g_hash_table_new(g_str_hash, g_str_equal);

	self->priv->volume = 50;
	self->priv->mute   = FALSE;
	self->priv->pl     = lomo_playlist_new();
	self->priv->meta   = lomo_meta_new(self);
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
	return lomo_player_play_stream(self, lomo_stream_new(uri), error);
}

gboolean
lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error)
{ TRACE
	g_return_val_if_fail(stream != NULL, FALSE);
		
	lomo_player_clear(self);
	lomo_player_add(self, stream);

	if (!lomo_player_reset(self, error) || !lomo_player_play(self, error))
		return FALSE;
	else
		return TRUE;
}

gboolean lomo_player_reset(LomoPlayer *self, GError **error)
{ TRACE
	GstElement *audio_sink;
	gint current = -1;

	// Destroy pipeline
	if (self->priv->pipeline != NULL)
	{
		gst_element_set_state(self->priv->pipeline, GST_STATE_NULL);
		g_object_unref(self->priv->pipeline);
		self->priv->pipeline = NULL;
	}

	// Sync current stream
	current = lomo_playlist_get_current(self->priv->pl);
	if (current == -1)
		self->priv->stream = NULL;
	else
		self->priv->stream = lomo_playlist_get_nth(self->priv->pl, current);
	
	// If there is no stream, nothing more to do.
	if (self->priv->stream == NULL)
		return TRUE;

	// g_printf("Set strema %p, uri: %s\n", self->priv->stream, (gchar*) lomo_stream_get_tag(self->priv->stream, LOMO_TAG_URI));
	// Sometimes stream tag's hasnt been parsed, in this case we move stream to
	// inmediate queue on LomoMeta object to get them ASAP
	if (!lomo_stream_has_all_tags(LOMO_STREAM(self->priv->stream)))
	{
		lomo_meta_parse(self->priv->meta, LOMO_STREAM(self->priv->stream), LOMO_META_PRIO_INMEDIATE);
	}

	// Now, create pipeline
	if ((self->priv->pipeline = gst_element_factory_make("playbin", "playbin")) == NULL)
	{
		g_printf("[liblomo (%s:%d)] Cannot create pipeline\n", __FILE__,__LINE__);
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_NO_PIPELINE,
			"[liblomo (%s:%d)] Cannot create pipeline\n", __FILE__,__LINE__);
		return FALSE;
	}

	// set audio-sink
	audio_sink = gst_element_factory_make(
			(gchar *) g_hash_table_lookup(self->priv->options, (gpointer) "audio-output"),
			"audio-sink");
	g_object_set(self->priv->pipeline, "audio-sink", audio_sink, NULL);

	// Attach a bus watch
	gst_bus_add_watch(
		gst_pipeline_get_bus(GST_PIPELINE(self->priv->pipeline)),
		_lomo_player_bus_watcher,
		self);

	// Setup pipeline
	g_object_set(self->priv->pipeline,
		"uri", g_object_get_data(G_OBJECT(self->priv->stream), "uri"),
		NULL);
	lomo_player_set_volume(self, self->priv->volume);
	lomo_player_set_mute  (self, self->priv->mute);
	
	GstStateChangeReturn r;
	r = gst_element_set_state (self->priv->pipeline, GST_STATE_READY);
	if (r == GST_STATE_CHANGE_FAILURE)
	{
		/// XXX: Emit some signal
		g_printf("ERROR: change state\n");
		return FALSE;
	}
	return TRUE;
}

LomoStream *lomo_player_get_stream(LomoPlayer *self)
{ TRACE
	// Hold this for a while to watch the DPP bug
	if (lomo_playlist_get_nth(self->priv->pl, lomo_playlist_get_current(self->priv->pl)) != self->priv->stream)
		g_printf("[liblomo (%s:%d)] DPP (desyncronized playlist and player) bug found\n", __FILE__, __LINE__);
	return self->priv->stream;
}

LomoStateChangeReturn lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error)
{ TRACE
    GstState gst_state;
    GstStateChangeReturn ret;

	// Unknow state
    if (!lomo_state_to_gst(state, &gst_state))
	{
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_UNKNOW_STATE,
			"Unknow state '%d'", state);
        return LOMO_STATE_CHANGE_FAILURE;
	}

	// No stream
	if (self->priv->stream == NULL)
	{
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_NO_STREAM,
			"No active stream");
		return LOMO_STATE_CHANGE_FAILURE;
	}

    switch (state)
	{
	case LOMO_STATE_PAUSE:
		ret = gst_element_set_state(self->priv->pipeline, GST_STATE_PAUSED);
		break;

	case LOMO_STATE_PLAY:
		ret = gst_element_set_state(self->priv->pipeline, GST_STATE_PLAYING);
		break;

	case LOMO_STATE_STOP:
		ret = gst_element_set_state(self->priv->pipeline, GST_STATE_NULL);
		lomo_player_reset(self, NULL);
		break;

	default:
		return LOMO_STATE_CHANGE_FAILURE;
		break;
	}

	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE,
			"Error while setting state on pipeline");
		return LOMO_STATE_CHANGE_FAILURE;
	}

	return LOMO_STATE_CHANGE_SUCCESS; // Or async, or preroll...
}

LomoState lomo_player_get_state(LomoPlayer *self)
{ TRACE
	GstState gst_state;
	LomoState lomo_state;
	
	if (self->priv->stream == NULL)
		return LOMO_STATE_STOP;

	if (gst_element_get_state(self->priv->pipeline, &gst_state, NULL, 0) ==
		GST_STATE_CHANGE_FAILURE)
		return LOMO_STATE_INVALID;

	lomo_state_from_gst(gst_state, &lomo_state);
	return lomo_state;
}

gint64 lomo_player_tell(LomoPlayer *self, LomoFormat format)
{ TRACE
	GstFormat gst_format_wanted, gst_format_used;
	gint64 val, ret;

	// No stream
	if (self->priv->stream == NULL)
		return -1;

	// Invalid format
	if (!lomo_format_to_gst(format, &gst_format_wanted)) {
		return -1;
	}

	// Unable to get positition
	gst_format_used = gst_format_wanted;
	if(!gst_element_query_position(
			GST_ELEMENT(self->priv->pipeline),
			&gst_format_used, &val))
	{
		return -1;
	}

	// Ok, format is correct
	if (gst_format_wanted == gst_format_used)
		return val;

	// Do a conversion if posible
	if (!gst_element_query_convert(
		GST_ELEMENT(self->priv->pipeline),
		gst_format_used, val,
		&gst_format_wanted, &ret))
		return -1;
	else
		return ret;
}

gboolean
lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val)
{ TRACE
	GstFormat gst_format;
	gint64 old;

	// No stream
	if ( self->priv->stream == NULL ) 
		return FALSE;

	// Incorrect format
	if (!lomo_format_to_gst(format, &gst_format)) {
		return FALSE;
	}

	old = lomo_player_tell(self, format);
	if (!gst_element_seek(
			GST_ELEMENT(self->priv->pipeline), 1.0,
			gst_format, GST_SEEK_FLAG_FLUSH,
			GST_SEEK_TYPE_SET, val,
			GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
		return FALSE;
	}

	g_signal_emit(G_OBJECT(self), lomo_player_signals[SEEK], 0, old, val);
	return TRUE;
}

gint64
lomo_player_length(LomoPlayer *self, LomoFormat format)
{ TRACE
	GstFormat gst_format;
	gint64 ret;

	// No stream
	if (self->priv->stream == NULL)
		return -1;

	// Invalid format
	if (!lomo_format_to_gst(format, &gst_format)) {
		return -1;
	}

	// Error quering duration
	if (!gst_element_query_duration(
		GST_ELEMENT(self->priv->pipeline), &gst_format, &ret))
		return -1;

	return ret;
}

gboolean lomo_player_set_volume(LomoPlayer *self, gint val)
{ TRACE
	gfloat _val;

	self->priv->volume = val;
	if (self->priv->pipeline == NULL)
		return TRUE;

	_val = (CLAMP(self->priv->volume, 0, 100)  * 0.01);
	g_object_set(G_OBJECT(self->priv->pipeline), "volume", _val, NULL);
	g_signal_emit(self, lomo_player_signals[VOLUME], 0, _val);

	return TRUE;
}

gint lomo_player_get_volume(LomoPlayer *self)
{ TRACE
	return self->priv->volume;
}

gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute)
{ TRACE
	gfloat _val;

	self->priv->mute = mute;
	if (self->priv->pipeline == NULL)
		return TRUE;

	if (mute)
		g_object_set(self->priv->pipeline, "volume", (gfloat) 0 , NULL); 
	else
	{
		_val = (CLAMP(self->priv->volume, 0, 100)  * 0.01);
		g_object_set(self->priv->pipeline, "volume", _val, NULL);
	}

	g_signal_emit(self, lomo_player_signals[MUTE], 0 , mute);
	return TRUE;
}

gboolean lomo_player_get_mute(LomoPlayer *self)
{ TRACE
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

		lomo_meta_parse(self->priv->meta, stream, LOMO_META_PRIO_DEFAULT);
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
	lomo_meta_clear(self->priv->meta);
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

/*
 * 
 * Watchers and private functions
 * 
 */

static gboolean
_lomo_player_bus_watcher(GstBus *bus, GstMessage *message, gpointer data)
{ TRACE
	LomoPlayer *self = LOMO_PLAYER(data);
	GError *err = NULL;
	gchar *debug = NULL;
	LomoStream *stream = NULL;

	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(message, &err, &debug);
			if ((stream = lomo_player_get_stream(self)) != NULL)
				lomo_stream_set_failed(stream, TRUE);
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

