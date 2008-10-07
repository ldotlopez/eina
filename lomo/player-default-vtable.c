#include "player-default-vtable.h"

GstPipeline*
default_create(GHashTable *opts)
{
	GstElement *ret, *audio_sink;
	const gchar *audio_sink_str;
	
	ret = gst_element_factory_make("playbin2", "playbin2");
	if (ret == NULL)
		return NULL;

	audio_sink_str = (gchar *) g_hash_table_lookup(opts, (gpointer) "audio-output");
	if (audio_sink_str == NULL)
		audio_sink_str = "autoaudiosink";
	
	audio_sink = gst_element_factory_make(audio_sink_str, "audio-sink"); 
	g_object_set(G_OBJECT(ret), "audio-sink", audio_sink, NULL);

	return GST_PIPELINE(ret);
}

void
default_destroy(GstPipeline *pipeline)
{
	g_object_unref(pipeline);
}

gboolean
default_set_stream(GstPipeline *pipeline, const gchar *uri)
{
	g_object_set(G_OBJECT(pipeline), "uri", uri, NULL);
	return TRUE;
}

GstStateChangeReturn
default_set_state(GstPipeline *pipeline, GstState state)
{
	return gst_element_set_state(GST_ELEMENT(pipeline), state);
}

GstState
default_get_state(GstPipeline *pipeline)
{
	GstState state, pending;
	gst_element_get_state(GST_ELEMENT(pipeline), &state, &pending, GST_CLOCK_TIME_NONE);
	return state;
}

gboolean
default_query_position(GstPipeline *pipeline, GstFormat *format, gint64 *position)
{
	return gst_element_query_position(GST_ELEMENT(pipeline), format, position);
}

gboolean
default_query_duration(GstPipeline *pipeline, GstFormat *format, gint64 *duration)
{
	return gst_element_query_duration(GST_ELEMENT(pipeline), format, duration);
}

gboolean
default_set_volume(GstPipeline *pipeline, gint volume)
{
	g_object_set(G_OBJECT(pipeline), "volume", volume, NULL);
	return TRUE;
}

gint
default_get_volume(GstPipeline *pipeline)
{
	gint volume;
	g_object_get(G_OBJECT(pipeline), "volume", &volume, NULL);
	return volume;
}

gboolean
default_set_mute(GstPipeline *pipeline, gboolean mute)
{
	g_object_set(G_OBJECT(pipeline), "mute", mute, NULL);
	return TRUE;
}

gboolean
default_get_mute(GstPipeline *pipeline)
{
	gboolean mute;
	g_object_get(G_OBJECT(pipeline), "mute", &mute, NULL);
	return mute;
}

