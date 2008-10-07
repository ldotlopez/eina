#include <gst/gst.h>
#include <lomo/player.h>

GstPipeline* default_create(GHashTable *opts);
void         default_destroy(GstPipeline *pipeline);

gboolean default_set_stream(GstPipeline *pipeline, const gchar *uri);

GstStateChangeReturn default_set_state(GstPipeline *pipeline, GstState state);
GstState             default_get_state(GstPipeline *pipeline);

gboolean default_query_position(GstPipeline *pipeline, GstFormat *format, gint64 *position);
gboolean default_query_duration(GstPipeline *pipeline, GstFormat *format, gint64 *duration);

gboolean default_set_volume(GstPipeline *pipeline, gint volume);
gint     default_get_volume(GstPipeline *pipeline);

gboolean default_set_mute(GstPipeline *pipeline, gboolean mute);
gboolean default_get_mute(GstPipeline *pipeline);

