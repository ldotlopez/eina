/*
 * lomo/lomo-metadata-parser.c
 *
 * Copyright (C) 2004-2011 Eina
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

#define DEBUG 0
#if DEBUG
#	define debug(...) g_debug(__VA_ARGS__)
#else
#	define debug(...) ;
#endif

#include "lomo-metadata-parser.h"

#include <glib/gi18n.h>
#include <gst/gst.h>
#include <gel/gel-misc.h>
#include "lomo-marshallers.h"

G_DEFINE_TYPE (LomoMetadataParser, lomo_metadata_parser, G_TYPE_OBJECT)

enum
{
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
	GQueue     *queue;    // Stream queue
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
	LomoMetadataParser *self = LOMO_METADATA_PARSER(object);
	LomoMetadataParserPrivate *priv = self->priv;

	if (priv->pipeline)
	{
		g_object_unref(priv->pipeline);
		priv->pipeline = NULL;
	}
	if (priv->queue)
	{
		g_queue_foreach(priv->queue, (GFunc) g_object_unref, NULL);
		g_queue_free(priv->queue);
		priv->queue = NULL;
	}
	if (G_OBJECT_CLASS (lomo_metadata_parser_parent_class)->dispose)
		G_OBJECT_CLASS (lomo_metadata_parser_parent_class)->dispose(object);
}

static void
lomo_metadata_parser_class_init (LomoMetadataParserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	/**
	 * LomoMetadataParser::tag:
	 * @parser: The parser
	 * @stream: (type Lomo.Stream): The stream where the tag was found
	 * @tag: The #LomoTag found
	 *
	 * Emitted for every tag found in the @stream
	 */
	lomo_metadata_parser_signals[TAG] =
		g_signal_new ("tag",
			G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoMetadataParserClass, tag),
			NULL, NULL,
			lomo_marshal_VOID__OBJECT_STRING,
			G_TYPE_NONE,
			2,
			G_TYPE_OBJECT,
			G_TYPE_STRING);

	/**
	 * LomoMetadataParser::all-tags:
	 * @parser: The parser
	 * @stream: (type Lomo.Stream): The stream where all tags have been parsed
	 *
	 * Emitted when all tags in the @stream have been parsed
	 */
	lomo_metadata_parser_signals[ALL_TAGS] =
		g_signal_new ("all-tags",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (LomoMetadataParserClass, all_tags),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE,
			1,
			G_TYPE_OBJECT);

	g_type_class_add_private (klass, sizeof (LomoMetadataParserPrivate));

	object_class->dispose = lomo_metadata_parser_dispose;
}

static void
lomo_metadata_parser_init (LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), LOMO_TYPE_METADATA_PARSER, LomoMetadataParserPrivate));
	priv->pipeline = NULL;
	priv->queue    = g_queue_new();
	priv->stream   = NULL;
	priv->failure  = priv->got_state_signal = priv->got_new_clock_signal = FALSE;
}

/**
 * lomo_metadata_parser_new:
 *
 * Creates a new #LomoMetadataParser object
 *
 * Returns: The object
 */
LomoMetadataParser*
lomo_metadata_parser_new (void)
{
	return g_object_new (LOMO_TYPE_METADATA_PARSER, NULL);
}

/**
 * lomo_metadata_parser_parse:
 * @self: The parser.
 * @stream: (transfer none): The stream to parse.
 * @prio: The priority on the queue.
 *
 * Adds @stream to @self internal queue with @prio to be parsed
 */
void
lomo_metadata_parser_parse(LomoMetadataParser *self, LomoStream *stream, LomoMetadataParserPrio prio)
{
	g_return_if_fail(LOMO_IS_METADATA_PARSER(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));
	g_return_if_fail((prio > LOMO_METADATA_PARSER_PRIO_INVALID) && (prio < LOMO_METADATA_PARSER_PRIO_N_PRIOS));

	LomoMetadataParserPrivate *priv = self->priv;
	switch (prio)
	{
		case LOMO_METADATA_PARSER_PRIO_INMEDIATE:
			g_queue_push_head(priv->queue, stream);
			break;
		case LOMO_METADATA_PARSER_PRIO_DEFAULT:
			g_queue_push_tail(priv->queue, stream);
			break;
		default:
			g_warning("Invalid priority %d\n", prio);
			return;
	}
	g_object_ref(stream);

	if (priv->idle_id == 0)
		priv->idle_id = g_idle_add((GSourceFunc) run_queue, self);
}

/**
 * lomo_metadata_parser_clear:
 * @self: The parser
 *
 * Clears internal queue and stop any parse in progress
 */
void
lomo_metadata_parser_clear(LomoMetadataParser *self)
{
	g_return_if_fail(LOMO_IS_METADATA_PARSER(self));

	LomoMetadataParserPrivate *priv = self->priv;

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

	if (priv->pipeline)
	{
		gst_element_set_state(priv->pipeline, GST_STATE_NULL);
		g_object_unref(priv->pipeline);
		priv->pipeline = NULL;
	}

	if (!g_queue_is_empty(priv->queue))
	{
		g_queue_foreach(priv->queue, (GFunc) g_object_unref, NULL);
		g_queue_clear(priv->queue);
	}

	priv->failure = priv->got_state_signal = priv->got_new_clock_signal = FALSE;
	priv->stream = NULL;
}

static void
reset_internals(LomoMetadataParser *self)
{
	g_return_if_fail(LOMO_IS_METADATA_PARSER(self));

	LomoMetadataParserPrivate *priv = self->priv;

	gel_object_free_and_invalidate(priv->pipeline);

	priv->stream = NULL;
	priv->failure = FALSE;
	priv->got_state_signal = FALSE;
	priv->got_new_clock_signal = FALSE;

	/* Generate pipeline */
	priv->pipeline = gst_element_factory_make ("playbin", "playbin");
	g_object_set (G_OBJECT (priv->pipeline),
		"audio-sink", gst_element_factory_make ("fakesink", "fakesink"),
		"video-sink", gst_element_factory_make ("fakesink", "fakesink"),
		NULL);

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
	LomoMetadataParserPrivate *priv = self->priv;
	GstState old, new, pending;
	GstTagList *tags = NULL;

	GstFormat duration_format = GST_FORMAT_TIME;
	gint64 duration = -1;

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
		gst_tag_list_free(tags);
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
	if ((priv->got_state_signal == TRUE) && (priv->got_new_clock_signal == TRUE))
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

	// Set duration on LomoStream
	if (gst_element_query_duration(priv->pipeline, &duration_format, &duration))
	{
		if (duration_format != GST_FORMAT_TIME)
		{
			g_warn_if_fail(duration_format == GST_FORMAT_TIME);
			duration = -1;
		}
		lomo_stream_set_length(priv->stream, duration);
	}
	else
		g_warning("Unable to query duration");

	gst_element_set_state(priv->pipeline, GST_STATE_NULL);

	// Final emission for URI, not sure why
	g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0,  priv->stream, LOMO_TAG_URI);

	// Emission for all-tags signal on LomoMetadataParser
	// XXX: Stream should also emit all-tags
	lomo_stream_set_all_tags_flag(priv->stream, TRUE);
	g_signal_emit(self, lomo_metadata_parser_signals[ALL_TAGS], 0, priv->stream);

	g_object_unref(priv->stream);

	// If there are more streams to parse schudele ourselves
	if (!g_queue_is_empty(priv->queue))
		priv->idle_id = g_idle_add((GSourceFunc) run_queue, self);

	return FALSE;
}

static void
foreach_tag_cb(const GstTagList *list, const gchar *tag, LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = self->priv;

	GValue value = { 0 };
	if (!gst_tag_list_copy_value(&value, list, tag))
		g_warning(_("Unable to copy GstTagList for %s"), tag);
	else
	{
		lomo_stream_set_tag(priv->stream, tag, &value);
		g_signal_emit(self, lomo_metadata_parser_signals[TAG], 0, priv->stream, tag);
		g_value_unset(&value);
	}
}

static gboolean
run_queue(LomoMetadataParser *self)
{
	LomoMetadataParserPrivate *priv = self->priv;

	if (!g_queue_is_empty(priv->queue))
	{
		reset_internals(self);
		priv->stream = g_queue_pop_head(priv->queue);
		g_object_set( G_OBJECT(priv->pipeline),
			"uri", lomo_stream_get_uri(priv->stream),
			NULL);
		gst_element_set_state(priv->pipeline, GST_STATE_PLAYING);
	}

	priv->idle_id = 0;
	return FALSE;
}


