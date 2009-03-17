/*
 * lomo/lomo-metadata-parser.c
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

#include <gst/gst.h>
#include <glib/gprintf.h>
#include <lomo/lomo-metadata-parser.h>
#include <lomo/lomo-marshallers.h>

G_DEFINE_TYPE (LomoMetadataParser, lomo_metadata_parser, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOMO_TYPE_METADATA_PARSER, LomoMetadataParserPrivate))

typedef struct _LomoMetadataParserPrivate LomoMetadataParserPrivate;

enum {
	TAG,
	ALL_TAGS,

	LAST_SIGNAL
};

guint lomo_metadata_parser_signals[LAST_SIGNAL] = { 0 };

static gboolean
bus_watcher(GstBus *bus, GstMessage *message, LomoMetadataParser *self);
static void
foreach_tag_cb(const GstTagList *list, const gchar *tag, LomoMetadataParser *self);
static gboolean
run_queue(LomoMetadataParser *self);

struct _LomoMetadataParserPrivate {
	GstElement *pipeline; // Our processing pipeline
	GList      *queue;    // Stream queue
	LomoStream *stream;   // Current stream

	// Indicators
	gboolean    failure;
	gboolean    got_state_signal;
	gboolean    got_new_clock_signal;

	// Watchers 
	guint       bus_id;
	guint       idle_id;
};

static void
lomo_metadata_parser_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (lomo_metadata_parser_parent_class)->dispose)
		G_OBJECT_CLASS (lomo_metadata_parser_parent_class)->dispose(object);
}

static void
lomo_metadata_parser_class_init (LomoMetadataParserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	lomo_metadata_parser_signals[TAG] =
		g_signal_new ("tag",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoMetadataParserClass, tag),
			NULL, NULL,
			lomo_marshal_VOID__POINTER_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER,
			G_TYPE_INT);
	lomo_metadata_parser_signals[ALL_TAGS] =
		g_signal_new ("all-tags",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoMetadataParserClass, all_tags),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER);

	g_type_class_add_private (klass, sizeof (LomoMetadataParserPrivate));

	object_class->dispose = lomo_metadata_parser_dispose;
}

static void
lomo_metadata_parser_init (LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = GET_PRIVATE(self);

	priv->pipeline = NULL;
	priv->queue    = NULL;
	priv->stream   = NULL;
	priv->failure  = priv->got_state_signal = priv->got_new_clock_signal = FALSE;
}

LomoMetadataParser*
lomo_metadata_parser_new (void)
{
	return g_object_new (LOMO_TYPE_METADATA_PARSER, NULL);
}

void
lomo_metadata_parser_parse(LomoMetadataParser *self, LomoStream *stream, LomoMetadataParserPrio prio)
{
	LomoMetadataParserPrivate *priv = priv = GET_PRIVATE(self);
	g_object_ref(stream);
	switch (prio)
	{
		case LOMO_METADATA_PARSER_PRIO_INMEDIATE:
			priv->queue = g_list_prepend(priv->queue, stream);
			break;
		case LOMO_METADATA_PARSER_PRIO_DEFAULT:
			priv->queue = g_list_append(priv->queue, stream);
			break;
		default:
			g_warning("Invalid priority %d\n", prio);
			return;
	}

	if (priv->idle_id == 0)
	{
		priv->idle_id = g_idle_add((GSourceFunc) run_queue, self);
	}
}

void
lomo_metadata_parser_clear(LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = GET_PRIVATE(self);
	if (!priv)
		return;

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
	{
		gst_element_set_state(priv->pipeline, GST_STATE_NULL);
		g_object_unref(priv->pipeline);
		priv->pipeline = NULL;
	}

	// Clear queue
	if (priv->queue)
	{
		g_list_foreach(priv->queue, (GFunc) g_object_unref, NULL);
		g_list_free(priv->queue);
		priv->queue = NULL;
	}

	// Reset flags and pointers
	priv->failure = priv->got_state_signal = priv->got_new_clock_signal = FALSE;
	priv->stream = NULL;
}

static void
lomo_metadata_parser_reset(LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = GET_PRIVATE(self);
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
		(GstBusFunc) bus_watcher, self);
}

static gboolean
bus_watcher (GstBus *bus, GstMessage *message, LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = GET_PRIVATE(self);
	GstState old, new, pending;
	GstTagList *tags = NULL;

	// Handle some messages
	switch (GST_MESSAGE_TYPE(message))
	{
	case GST_MESSAGE_ERROR:
	case GST_MESSAGE_EOS:
		priv->failure = TRUE;
		break;

	case GST_MESSAGE_TAG:
		gst_message_parse_tag(message, &tags);
		gst_tag_list_foreach(tags, (GstTagForeachFunc) foreach_tag_cb, (gpointer) self);
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

	// In case of failure disconnect from bus
	if (priv->failure)
		goto disconnect;

	// Got the state-change and new-clock signal, this means that all is ok.
	// We can disconnect
	if ( (priv->got_state_signal == TRUE) && (priv->got_new_clock_signal == TRUE)) 
		goto disconnect;

	// Stay on the bus
	return TRUE;

	/*
	 * Common operations on disconnect from bus:
	 * 1. Stop processing pipeline
	 * 2. Emit the ALL_TAGS signal
	 * 3. Unref the stream
	 * 4. Check if more streams are in queue and start again
	 */
disconnect:
	gst_element_set_state(priv->pipeline, GST_STATE_NULL);

	lomo_stream_set_tag(priv->stream, LOMO_TAG_URI, g_strdup(lomo_stream_get_tag(priv->stream, LOMO_TAG_URI)));
	g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0,  priv->stream, LOMO_TAG_URI);

	lomo_stream_set_all_tags_flag(priv->stream, TRUE);
	g_signal_emit(self, lomo_metadata_parser_signals[ALL_TAGS], 0, priv->stream);
	g_object_unref(priv->stream);

	// If there are more streams to parse schudele ourselves
	if (priv->queue != NULL)
		priv->idle_id = g_idle_add((GSourceFunc) run_queue, self);

	return FALSE;
}

static void
foreach_tag_cb(const GstTagList *list, const gchar *tag, LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = GET_PRIVATE(self);
	GType     type;

	gchar   *string_tag;
	gint     int_tag;
	guint    uint_tag;
	guint64  uint64_tag;
	gboolean bool_tag;
	gpointer pointer;

	type = lomo_tag_get_type(tag);

	switch (type)
	{
		case G_TYPE_STRING:
			if (!gst_tag_list_get_string(list, tag, &string_tag))
				break;

			lomo_stream_set_tag(priv->stream, tag, g_strdup(string_tag));
			g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0, priv->stream, tag);
			break;

		case G_TYPE_UINT:
			if (!gst_tag_list_get_uint(list, tag, &uint_tag))
				break;

			pointer = g_new0(guint, 1);
			*((guint*) pointer) = uint_tag;
			lomo_stream_set_tag(priv->stream, tag, pointer);
			g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0, priv->stream, tag);
			break;

		case G_TYPE_UINT64:
			if (!gst_tag_list_get_uint64(list, tag, &uint64_tag))
				break;
			pointer = g_new0(guint64, 1);
			*((guint64*) pointer) = uint64_tag;
			lomo_stream_set_tag(priv->stream, tag, pointer);
			g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0, priv->stream, tag);
			break;

		case G_TYPE_INT:
			if (!gst_tag_list_get_int(list, tag, &int_tag))
				break;
			pointer = g_new0(gint, 1);
			*((guint*) pointer) = int_tag;
			lomo_stream_set_tag(priv->stream, tag, pointer);
			g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0, priv->stream, tag);
			break;

		case G_TYPE_BOOLEAN:
			if (!gst_tag_list_get_boolean(list, tag, &bool_tag))
				break;
			pointer = g_new0(gboolean, 1);
			*((gboolean*) pointer) = bool_tag;
			lomo_stream_set_tag(priv->stream, tag, pointer);
			g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0, priv->stream, tag);
			break;

		default:
			/*
			meta.c:364 Found date but type GstDate is not handled
			meta.c:364 Found private-id3v2-frame but type GstBuffer is not handled

			g_printf("%s:%d Found %s but type %s is not handled\n",
				__FILE__, __LINE__, tag, g_type_name(type));
			*/
			break;
	}
}

static gboolean
run_queue(LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = GET_PRIVATE(self);
	
	if (priv->queue != NULL)
	{
		lomo_metadata_parser_reset(self);
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


