#include "player-default-vtable.h"
#define ERROR_DOMAIN "Lomo2PlayerCore"

enum {
	UNKNOW
};

GstElement*
default_create(GHashTable *opts, GError **error)
{
	GstElement *ret, *audio_sink;
	const gchar *audio_sink_str;
	
	ret = gst_element_factory_make("playbin2", "playbin2");
	if (ret == NULL)
	{
		if (error != NULL)
			*error = g_error_new(g_quark_from_static_string(ERROR_DOMAIN), UNKNOW, "Cannot create pipeline");
		return NULL;
	}

	audio_sink_str = (gchar *) g_hash_table_lookup(opts, (gpointer) "audio-output");
	if (audio_sink_str == NULL)
		audio_sink_str = "autoaudiosink";
	
	audio_sink = gst_element_factory_make(audio_sink_str, "audio-sink");
	if (audio_sink == NULL)
	{
		g_object_unref(ret);
		*error = g_error_new(g_quark_from_static_string(ERROR_DOMAIN), UNKNOW, "Cannot create audio-sink %s", audio_sink_str);
	}
	g_object_set(G_OBJECT(ret), "audio-sink", audio_sink, NULL);

	return ret;
}

gboolean
default_destroy(GstElement *pipeline, GError **error)
{
	g_object_unref(G_OBJECT(pipeline));
	return TRUE;
}

gboolean
default_set_stream(GstElement *pipeline, const gchar *uri)
{
	g_object_set(G_OBJECT(pipeline), "uri", uri, NULL);
	return TRUE;
}

GstStateChangeReturn
default_set_state(GstElement *pipeline, GstState state)
{
	return gst_element_set_state(pipeline, state);
}

GstState
default_get_state(GstElement *pipeline)
{
	GstState state, pending;
	gst_element_get_state(pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
	return state;
}

gboolean
default_query_position(GstElement *pipeline, GstFormat *format, gint64 *position)
{
	return gst_element_query_position(pipeline, format, position);
}

gboolean
default_query_duration(GstElement *pipeline, GstFormat *format, gint64 *duration)
{
	return gst_element_query_duration(pipeline, format, duration);
}

gboolean
default_set_volume(GstElement *pipeline, gint volume)
{
	g_object_set(G_OBJECT(pipeline), "volume", volume, NULL);
	return TRUE;
}

gint
default_get_volume(GstElement *pipeline)
{
	gint volume;
	g_object_get(G_OBJECT(pipeline), "volume", &volume, NULL);
	return volume;
}

gboolean
default_set_mute(GstElement *pipeline, gboolean mute)
{
	g_object_set(G_OBJECT(pipeline), "mute", mute, NULL);
	return TRUE;
}

gboolean
default_get_mute(GstElement *pipeline)
{
	gboolean mute;
	g_object_get(G_OBJECT(pipeline), "mute", &mute, NULL);
	return mute;
}

