#include <gst/gst.h>
#include <lomo/player.h>

GstElement* default_create(GHashTable *opts, GError **error);
gboolean     default_destroy(GstElement *pipeline, GError **error);

gboolean default_set_stream(GstElement *pipeline, const gchar *uri);

GstStateChangeReturn default_set_state(GstElement *pipeline, GstState state);
GstState             default_get_state(GstElement *pipeline);

gboolean default_query_position(GstElement *pipeline, GstFormat *format, gint64 *position);
gboolean default_query_duration(GstElement *pipeline, GstFormat *format, gint64 *duration);

gboolean default_set_volume(GstElement *pipeline, gint volume);
gint     default_get_volume(GstElement *pipeline);

gboolean default_set_mute(GstElement *pipeline, gboolean mute);
gboolean default_get_mute(GstElement *pipeline);

