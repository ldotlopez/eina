#include <gst/gst.h>
#include <glib/gprintf.h>
#include "player.h"
#include "player-priv.h"
#include "pl.h"
#include "meta.h"
#include "lomo-marshal.h"
#include "util.h"

#ifdef LOMO_DEBUG
#define BACKTRACE g_printf("[LomoPlayer Backtrace] %s %d\n", __FUNCTION__, __LINE__);
#else
#define BACKTRACE ((void)(0));
#endif

#define lomo_player_set_error(error,code,...) \
	do { \
		if (error) \
			*error = g_error_new(lomo_player_domain,code,__VA_ARGS__); \
	} while(0)

G_DEFINE_TYPE(LomoPlayer, lomo_player, G_TYPE_OBJECT)

static GQuark lomo_player_domain;
enum {
	LOMO_PLAYER_ERROR_NO_PIPELINE = 1,
	LOMO_PLAYER_ERROR_NO_STREAM,
	LOMO_PLAYER_ERROR_UNKNOW_STATE,
	LOMO_PLAYER_ERROR_CHANGE_STATE_FAILURE,
	LOMO_PLAYER_ERROR_STATE_FAILURE,
	LOMO_PLAYER_ERROR_GENERIC
};

static gboolean _lomo_player_bus_watcher (
	GstBus *bus,
	GstMessage *message,
	gpointer data);

struct _LomoPlayerPrivate {
	const LomoStream *stream;

	GstElement *pipeline;
	gchar      *audio_output_str;

	LomoPlaylist *pl;
	LomoMeta     *meta;
	gint          volume;
	gboolean      mute;
};

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_PLAYER, LomoPlayerPrivate))

void
lomo_init(gint *argc, gchar **argv[])
{ BACKTRACE
	gst_init(argc, argv);
	lomo_player_domain = g_quark_from_string("LomoPlayer");
}

static void
lomo_player_dispose(GObject *object)
{ BACKTRACE
	LomoPlayer *self;

	self = LOMO_PLAYER (object);

	if (self->priv->pipeline) {
		gst_element_set_state(self->priv->pipeline, GST_STATE_NULL);
		g_object_unref(self->priv->pipeline);
	}

	if (self->priv->pl) {
		lomo_playlist_unref(self->priv->pl);
		self->priv->pl = NULL;
	}
	if (self->priv->meta) {
		g_object_unref(self->priv->meta);
		self->priv->meta = NULL;
	}
	if (self->priv->audio_output_str)
	{
		g_free(self->priv->audio_output_str);
		self->priv->audio_output_str = NULL;
	}
	G_OBJECT_CLASS (lomo_player_parent_class)->dispose (object);
}

static void
lomo_player_class_init (LomoPlayerClass *klass)
{ BACKTRACE
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
			      lomo_marshal_VOID__POINTER_STRING,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_POINTER,
				  G_TYPE_STRING);
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
{ BACKTRACE
	self->priv = GET_PRIVATE(self);
	self->priv->volume = 50;
	self->priv->mute   = FALSE;
	self->priv->pl     = lomo_playlist_new();
	self->priv->meta   = lomo_meta_new(self);
}

/*
 *
 * Core functions
 *
 */
LomoPlayer*
lomo_player_new(gchar *audio_output, GError **error)
{ BACKTRACE
	LomoPlayer *self;

	self = g_object_new (LOMO_TYPE_PLAYER, NULL, NULL);
	self->priv->audio_output_str = g_strdup(audio_output);
	return self;
}

/*
 * Quick play functions, simple shortcuts.
 */
void lomo_player_play_uri(LomoPlayer *self, gchar *uri, GError **error)
{ BACKTRACE
	lomo_player_play_stream(self, lomo_stream_new(uri), error);
}

void lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error)
{ BACKTRACE
	lomo_player_clear(self);
	lomo_player_add(self, stream);
	lomo_player_reset(self, NULL);
	lomo_player_play(self, NULL);
}

// LomoOperations vtable
gboolean lomo_player_reset(LomoPlayer *self, GError **error)
{ BACKTRACE
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

	// Sometimes stream tag's hasnt been parsed, in this case we move stream to
	// inmediate queue on LomoMeta object to get them ASAP
	if (!lomo_stream_has_all_tags(LOMO_STREAM(self->priv->stream)))
	{
		g_printf("Stream '%s' has no tags!\n", (gchar *) lomo_stream_get_tag(self->priv->stream, LOMO_TAG_URI));
		lomo_meta_parse(self->priv->meta, LOMO_STREAM(self->priv->stream), LOMO_META_PRIO_INMEDIATE);
	}

	// Now, create pipeline
	if ((self->priv->pipeline = gst_element_factory_make("playbin2", "playbin2")) == NULL)
	{
		g_printf("[liblomo (%s:%d)] Cannot create pipeline\n", __FILE__,__LINE__);
		lomo_player_set_error(error, LOMO_PLAYER_ERROR_NO_PIPELINE,
			"[liblomo (%s:%d)] Cannot create pipeline\n", __FILE__,__LINE__);
		return FALSE;
	}

	// set audio-sink
	audio_sink = gst_element_factory_make(
			self->priv->audio_output_str, "audio-sink");
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
	gst_element_set_state (self->priv->pipeline, GST_STATE_READY);

	return TRUE;
}

const LomoStream *lomo_player_get_stream(LomoPlayer *self)
{ BACKTRACE
	// Put this for a while to watch the DPP bug
	if (lomo_playlist_get_nth(self->priv->pl, lomo_playlist_get_current(self->priv->pl)) != self->priv->stream)
		g_printf("[liblomo (%s:%d)] DPP (desyncronized playlist and player) bug found\n", __FILE__, __LINE__);
	return self->priv->stream;
}

LomoStateChangeReturn lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error)
{ BACKTRACE
    GstState gst_state;
    GstStateChangeReturn ret;
	guint signal_to_emit = 0;

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
		signal_to_emit = PAUSE;
		// g_signal_emit(G_OBJECT(self), lomo_player_signals[PAUSE], 0);
		break;

	case LOMO_STATE_PLAY:
		ret = gst_element_set_state(self->priv->pipeline, GST_STATE_PLAYING);
		signal_to_emit = PLAY;
		// g_signal_emit(G_OBJECT(self), lomo_player_signals[PLAY], 0);
		break;

	case LOMO_STATE_STOP:
		ret = gst_element_set_state(self->priv->pipeline, GST_STATE_NULL);
		lomo_player_reset(self, NULL);
		signal_to_emit = STOP;
		// g_signal_emit(G_OBJECT(self), lomo_player_signals[STOP], 0);
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
	g_signal_emit(G_OBJECT(self), lomo_player_signals[signal_to_emit], 0);
	return LOMO_STATE_CHANGE_SUCCESS;
}

LomoState lomo_player_get_state(LomoPlayer *self)
{ BACKTRACE
	GstState gst_state;
	LomoState lomo_state;
	
	if (self->priv->stream == NULL)
		return LOMO_STATE_STOP;

	if (gst_element_get_state(self->priv->pipeline, &gst_state, NULL, GST_CLOCK_TIME_NONE) ==
		GST_STATE_CHANGE_FAILURE)
		return LOMO_STATE_INVALID;

	lomo_state_from_gst(gst_state, &lomo_state);
	return lomo_state;
}

gint64 lomo_player_tell(LomoPlayer *self, LomoFormat format)
{ BACKTRACE
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

gboolean lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val)
{ BACKTRACE
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

gint64 lomo_player_length(LomoPlayer *self, LomoFormat format)
{ BACKTRACE
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
{ BACKTRACE
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
{ BACKTRACE
	return self->priv->volume;
}

gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute)
{ BACKTRACE
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
{ BACKTRACE
	return self->priv->mute;
}

/*
 * Playlist functions
 */
gint lomo_player_add_at_pos(LomoPlayer *self, LomoStream *stream, gint pos)
{ BACKTRACE
	GList *tmp = NULL;
	gint ret;

	tmp = g_list_prepend(tmp, stream);
	ret = lomo_player_add_multi_at_pos(self, tmp, pos);
	g_list_free(tmp);

	return ret;
}

gint lomo_player_add_uri_multi_at_pos(LomoPlayer *self, GList *uris, gint pos)
{ BACKTRACE
	GList *l, *streams = NULL;
	gint ret;

	l = uris;
	while (l) {
		streams = g_list_append(streams, lomo_stream_new((gchar *) l->data));
		l = l->next;
	}

	ret = lomo_player_add_multi_at_pos(self, streams, pos);
	g_list_free(streams);

	return ret;
}

gint lomo_player_add_uri_strv_at_pos(LomoPlayer *self, gchar **uris, gint pos)
{ BACKTRACE
	GList *l = NULL;
	gint ret, i;

	for (i = 0; uris[i] != NULL; i++)
		l = g_list_prepend(l, uris[i]);
	l = g_list_reverse(l);
	ret = lomo_player_add_uri_multi_at_pos(self, l, pos);
	g_list_free(l);

	return ret;
}

gint lomo_player_add_multi_at_pos(LomoPlayer *self, GList *streams, gint pos)
{ BACKTRACE
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
{ BACKTRACE
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

const GList *lomo_player_get_playlist(LomoPlayer *self)
{ BACKTRACE
	return lomo_playlist_get_playlist(self->priv->pl);
}

const LomoStream *lomo_player_get_nth(LomoPlayer *self, gint pos)
{ BACKTRACE
	return lomo_playlist_get_nth(self->priv->pl, pos);
}

gint lomo_player_get_prev(LomoPlayer *self)
{ BACKTRACE
	return lomo_playlist_get_prev(self->priv->pl);
}

gint lomo_player_get_next(LomoPlayer *self)
{ BACKTRACE
	return lomo_playlist_get_next(self->priv->pl);
}

gboolean lomo_player_go_nth(LomoPlayer *self, gint pos, GError **error)
{ BACKTRACE
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
{ BACKTRACE
	return lomo_playlist_get_current(self->priv->pl);
}

guint lomo_player_get_total(LomoPlayer *self)
{ BACKTRACE
	return lomo_playlist_get_total(self->priv->pl);
}

void lomo_player_clear(LomoPlayer *self)
{ BACKTRACE
	lomo_player_stop(self, NULL);
	lomo_playlist_clear(self->priv->pl);
	lomo_meta_clear(self->priv->meta);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[CLEAR], 0);
}

void lomo_player_set_repeat(LomoPlayer *self, gboolean val)
{ BACKTRACE
	lomo_playlist_set_repeat(self->priv->pl, val);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[REPEAT], 0, val);
}

gboolean lomo_player_get_repeat(LomoPlayer *self)
{ BACKTRACE
	return lomo_playlist_get_repeat(self->priv->pl);
}

void lomo_player_set_random(LomoPlayer *self, gboolean val)
{ BACKTRACE
	lomo_playlist_set_random(self->priv->pl, val);
	g_signal_emit(G_OBJECT(self), lomo_player_signals[RANDOM], 0, val);
}

gboolean lomo_player_get_random(LomoPlayer *self)
{ BACKTRACE
	return lomo_playlist_get_random(self->priv->pl);
}

void lomo_player_randomize(LomoPlayer *self)
{ BACKTRACE
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

static gboolean _lomo_player_bus_watcher(GstBus *bus, GstMessage *message, gpointer data) {
	LomoPlayer *self = LOMO_PLAYER(data);
	GError *err = NULL;
	gchar *debug = NULL;

	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(message, &err, &debug);
			g_signal_emit(G_OBJECT(self), lomo_player_signals[ERROR], 0, err, debug);
			g_error_free(err);
			g_free(debug);
			break;

		case GST_MESSAGE_EOS:
			g_signal_emit(G_OBJECT(self), lomo_player_signals[EOS], 0);
			break;

		case GST_MESSAGE_TAG: /* Handled */
		case GST_MESSAGE_STATE_CHANGED:
		case GST_MESSAGE_CLOCK_PROVIDE:
		case GST_MESSAGE_CLOCK_LOST:
		case GST_MESSAGE_UNKNOWN:
		case GST_MESSAGE_WARNING:
		case GST_MESSAGE_INFO:
		case GST_MESSAGE_BUFFERING:
		case GST_MESSAGE_STATE_DIRTY:
		case GST_MESSAGE_STEP_DONE:
		case GST_MESSAGE_NEW_CLOCK:
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

