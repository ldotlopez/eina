#include "meta.h"
#include "player-priv.h"

G_DEFINE_TYPE (LomoMeta, lomo_meta, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_META, LomoMetaPrivate))

typedef struct _LomoMetaPrivate LomoMetaPrivate;

struct _LomoMetaPrivate {
	LomoPlayer *player;   // The LomoPlayer parent
	GstElement *pipeline; // Our processing pipeline
	GList      *queue;    // Stream queue
	LomoStream *stream;   // Current stream

	/* Indicators */
	gboolean    failure;
	gboolean    got_state_signal;
	gboolean    got_new_clock_signal;

	/* Watchers */
	guint       bus_id;
	guint       idle_id;
};

static void
lomo_meta_reset(LomoMeta *self);

static gboolean lomo_meta_bus_watcher
(GstBus *bus, GstMessage *message, LomoMeta *self);

static void lomo_meta_foreach_tag_cb
(const GstTagList *list, const gchar *tag, LomoMeta *self);

static gboolean lomo_meta_run
(LomoMeta *self);

static void
lomo_meta_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (lomo_meta_parent_class)->dispose)
		G_OBJECT_CLASS (lomo_meta_parent_class)->dispose (object);
}

static void
lomo_meta_class_init (LomoMetaClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoMetaPrivate));

	object_class->dispose = lomo_meta_dispose;
}

static void
lomo_meta_init (LomoMeta *self)
{
	struct _LomoMetaPrivate *priv = GET_PRIVATE(self);

	priv->player   = NULL;
	priv->pipeline = NULL;
	priv->queue = NULL;
	priv->stream   = NULL;
	priv->failure = priv->got_state_signal = priv->got_new_clock_signal = FALSE;
}

LomoMeta*
lomo_meta_new (LomoPlayer *player)
{
	LomoMeta *self;
	struct _LomoMetaPrivate *priv;

	self = g_object_new (LOMO_TYPE_META, NULL);
	priv = GET_PRIVATE(self);

	priv->player = player;

	return self;
}

void
lomo_meta_parse(LomoMeta *self, LomoStream *stream, LomoMetaPrio prio)
{
	struct _LomoMetaPrivate *priv = priv = GET_PRIVATE(self);
	g_object_ref(stream);
	switch (prio)
	{
		case LOMO_META_PRIO_INMEDIATE:
			priv->queue = g_list_prepend(priv->queue, stream);
			break;
		case LOMO_META_PRIO_DEFAULT:
			priv->queue = g_list_append(priv->queue, stream);
			break;
		default:
			g_warning("Invalid priority %d\n", prio);
			return;
	}

	if (priv->idle_id == 0)
	{
		priv->idle_id = g_idle_add((GSourceFunc) lomo_meta_run, self);
	}
}

#if 0
void lomo_meta_parse
(LomoMeta *self, LomoStream *stream, LomoMetaPrio prio)
{
	/* XXX: 101% posibilities of race condition */

	/*
	 * 1. Ref the object
	 * 2. Add to queue
	 * 3. Schudele parser
	 */
	g_object_ref(G_OBJECT(stream));
	self->queue = g_list_append(self->queue, stream);
	if (self->idle_id == 0) {
		// g_printf("[Meta] Schudele a run from %s\n", __FUNCTION__);
		self->idle_id = g_idle_add((GSourceFunc) _lomo_meta_run, self);
	}
}
#endif

void
lomo_meta_clear(LomoMeta *self)
{
	struct _LomoMetaPrivate *priv = GET_PRIVATE(self);

	// Stop the parser and bus
	if (priv->idle_id > 0)
	{
		g_source_remove(priv->idle_id);
		priv->idle_id = 0;
	}
	if (priv->bus_id > 0)
	{
		g_source_remove(priv->bus_id);
		priv->bus_id = 0;
	}

	// Clear pipeline
	if (priv->pipeline)
		g_object_unref(priv->pipeline);

	// Clear queue
	g_list_foreach(priv->queue, (GFunc) g_object_unref, NULL);
	g_list_free(priv->queue);
	priv->queue = NULL;

	// Reset flags and pointers
	priv->failure = priv->got_state_signal = priv->got_new_clock_signal = FALSE;
	priv->stream = NULL;
}

static void
lomo_meta_reset(LomoMeta *self)
{
	struct _LomoMetaPrivate *priv = GET_PRIVATE(self);
	GstElement *audio, *video;

	if (priv->pipeline)
		g_object_unref(priv->pipeline);

	priv->stream = NULL;
	priv->failure = FALSE;
	priv->got_state_signal = FALSE;
	priv->got_new_clock_signal = FALSE;

	/* Generate pipeline */
	audio   = gst_element_factory_make ("fakesink", "fakesink");
	video   = gst_element_factory_make ("fakesink", "fakesink");
	priv->pipeline     = gst_element_factory_make ("playbin", "playbin");
	g_object_set(G_OBJECT(priv->pipeline), "audio-sink", audio, NULL);
	g_object_set(G_OBJECT(priv->pipeline), "video-sink", video, NULL);

	/* Add watcher to bus */
	if (priv->bus_id > 0)
		g_source_remove(priv->bus_id);

	priv->bus_id = gst_bus_add_watch(
		gst_pipeline_get_bus(GST_PIPELINE(priv->pipeline)),
		(GstBusFunc) lomo_meta_bus_watcher, self);
}

static gboolean lomo_meta_bus_watcher
(GstBus *bus, GstMessage *message, LomoMeta *self)
{
	struct _LomoMetaPrivate *priv = GET_PRIVATE(self);
	GstState old, new, pending;
	GstTagList *tags = NULL;

#if DURATION_TAG_HACK
	guint64  duration;
	guint64 *duration_prt;
#endif

	/*
	 * Handle some messages
	 */
	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
		case GST_MESSAGE_EOS:
			priv->failure = TRUE;
			break;

		case GST_MESSAGE_TAG:
			gst_message_parse_tag(message, &tags);
			gst_tag_list_foreach(tags, (GstTagForeachFunc) lomo_meta_foreach_tag_cb, (gpointer) self);
			break;

		case GST_MESSAGE_STATE_CHANGED:
			gst_message_parse_state_changed(message, &old, &new, &pending);
			if ((old == GST_STATE_READY) && (new = GST_STATE_PAUSED)) {
				if (priv->got_state_signal == FALSE) 
					priv->got_state_signal = TRUE;
			}
			break;

		case GST_MESSAGE_NEW_CLOCK:
			if (priv->got_new_clock_signal == FALSE)
				priv->got_new_clock_signal = TRUE;
			break;

		default:
			break;
	}

	/*
	 * In case of failure disconnect from bus
	 */
	if (priv->failure) {
		goto disconnect;
	}

	/*
	 * Got the state-change and new-clock signal, this means that all is ok.
	 * We can disconnect
	 */
	if (
		(priv->got_state_signal == TRUE) &&
		(priv->got_new_clock_signal == TRUE)) {
		goto disconnect;
	}

	/*
	 * Stay on the bus
	 */
	return TRUE;

	/*
	 * Common operations on disconnect from bus:
	 * 1. Stop processing pipeline
	 * 2. Emit the ALL_TAGS signal
	 * 3. Unref the stream
	 * 4. Check if more streams are in queue and start again
	 */
disconnect:
#if DURATION_TAG_HACK
	/* If we dont have LOMO_TAG_DURATION defined from a tag
	 * get it from stream unsigned 64-bit.
	 */
	duration_prt = lomo_stream_get(priv->stream, LOMO_TAG_DURATION);
	if (duration_prt == NULL) {
		duration = lomo_player_length_time(priv->player);
		g_printf("Duration is not set => %lld\n", duration);
		duration_prt = g_new0(guint64, 1);
		* ((guint64*) duration_prt) = duration;
		g_object_set_data_full(G_OBJECT(priv->stream),
			LOMO_TAG_DURATION, duration_prt,
			NULL);
		g_signal_emit(
			G_OBJECT(priv->player), lomo_player_signals[TAG],
			0, priv->stream, LOMO_TAG_DURATION);
	}
#endif
	gst_element_set_state(priv->pipeline, GST_STATE_NULL);
	lomo_stream_set_all_tags(priv->stream, TRUE);
	g_signal_emit(
		G_OBJECT(priv->player), lomo_player_signals[ALL_TAGS],
		0, priv->stream);
	g_object_unref(priv->stream);

	// If there are more streams to parse schudele ourselves
	if (priv->queue != NULL) {
		priv->idle_id = g_idle_add((GSourceFunc) lomo_meta_run, self);
	}

	return FALSE;
}
static void lomo_meta_foreach_tag_cb
(const GstTagList *list, const gchar *tag, LomoMeta *self)
{
	struct _LomoMetaPrivate *priv = GET_PRIVATE(self);
	GType     type;

	gchar   *string_tag;
	gint     int_tag;
	guint    uint_tag;
	guint64  uint64_tag;
	gpointer pointer;

	type = lomo_tag_get_type(tag);

	switch (type) {
		case G_TYPE_STRING:
			if (gst_tag_list_get_string(list, tag, &string_tag)) {
				g_object_set_data_full(
						G_OBJECT(priv->stream),
						tag, g_strdup(string_tag),
						g_free);
				g_signal_emit(
						G_OBJECT(priv->player), lomo_player_signals[TAG],
						0, priv->stream, tag);
				g_free(string_tag);
			}
			break;

		case G_TYPE_UINT:
			if (gst_tag_list_get_uint(list, tag, &uint_tag)) {
				g_object_set_data(
						G_OBJECT(priv->stream),
						tag, (gpointer) uint_tag);
				g_signal_emit(
						G_OBJECT(priv->player), lomo_player_signals[TAG],
						0, priv->stream, tag);
			}
			break;

		case G_TYPE_UINT64:
			if (gst_tag_list_get_uint64(list, tag, &uint64_tag)) {
				pointer = g_new0(guint64, 1);
				* ((guint64*) pointer) = uint64_tag;
				g_object_set_data_full(
					G_OBJECT(priv->stream),
					tag, pointer, g_free);
				g_signal_emit(
					G_OBJECT(priv->player), lomo_player_signals[TAG],
					0, priv->stream, tag);
			}
			break;

		case G_TYPE_INT:
			if (gst_tag_list_get_int(list, tag, &int_tag)) {
				g_object_set_data(
						G_OBJECT(priv->stream),
						tag, (gpointer *) int_tag);
				g_signal_emit(
						G_OBJECT(priv->player), lomo_player_signals[TAG],
						0, priv->stream, tag);
			}
			break;

		default:
			/*
			g_printf("%s:%d Found %s but type %s is not handled\n",
				__FILE__, __LINE__, tag, g_type_name(type));
			*/
			break;
	}
}

/*
 * The '_run' hook.
 * Checks if there are streams on the queue. If more streams need to
 * be parsed, the first one is extracted from queue and put on self->stream
 * and metaline is launched.
 * This funcion allways erases himself from the idle
 */
static gboolean lomo_meta_run
(LomoMeta *self)
{
	struct _LomoMetaPrivate *priv = GET_PRIVATE(self);
	
	if (priv->queue != NULL) {
		lomo_meta_reset(self);
		priv->stream = (LomoStream *) priv->queue->data;
		priv->queue = priv->queue->next;
		g_object_set(
			G_OBJECT(priv->pipeline), "uri",
			g_object_get_data(G_OBJECT(priv->stream), LOMO_TAG_URI), NULL);
		gst_element_set_state(priv->pipeline, GST_STATE_PLAYING);
	}

	priv->idle_id = 0;
	return FALSE;
}

